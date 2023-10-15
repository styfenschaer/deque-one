#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include <stdint.h>

#define BLOCK_LOG_SIZE 6
#define BLOCK_SIZE (1 << BLOCK_LOG_SIZE)
#define NUM_INIT_SLOTS 16
#define GROWTH_RATE 2.0
#define SHRINK_RATE (1.0 / GROWTH_RATE)
#define MAX_FREE_BLOCKS 8

typedef PyObject** block_t;

typedef struct
{
    PyObject_VAR_HEAD
    Py_ssize_t left_item;
    Py_ssize_t left_block;
    block_t* blocks;
    Py_ssize_t right_block;
    Py_ssize_t right_item;
    Py_ssize_t maxlen;
    Py_ssize_t num_blocks;
    Py_ssize_t num_slots;
    block_t free_blocks[MAX_FREE_BLOCKS];
    Py_ssize_t num_free_blocks;
    PyObject* weakreflist;
} deque_t;

static inline void
Deque_PHYSICAL_INDEX(deque_t* self, Py_ssize_t i, Py_ssize_t* block_index, Py_ssize_t* data_index)
{
    i = i + self->left_item;
    *block_index = (i >> BLOCK_LOG_SIZE) + self->left_block;
    *data_index = i & (BLOCK_SIZE - 1);
}

static inline int
Deque_CHECK_INDEX(Py_ssize_t index, Py_ssize_t limit)
{
    return (size_t)index < (size_t)limit;
}

static inline PyObject*
Deque_GET_LEFT_ITEM(deque_t* self)
{
    return self->blocks[self->left_block][self->left_item];
}

static inline PyObject*
Deque_GET_RIGHT_ITEM(deque_t* self)
{
    return self->blocks[self->right_block][self->right_item];
}

static inline void
Deque_SET_RIGHT_ITEM(deque_t* self, PyObject* item)
{
    self->blocks[self->right_block][self->right_item] = item;
}

static inline void
Deque_SET_LEFT_ITEM(deque_t* self, PyObject* item)
{
    self->blocks[self->left_block][self->left_item] = item;
}

static PyObject*
deque_item(deque_t* self, Py_ssize_t i)
{
    Py_ssize_t block_index, data_index;

    if (!Deque_CHECK_INDEX(i, Py_SIZE(self))) {
        PyErr_SetString(PyExc_IndexError, "deque index out of range");
        return NULL;
    }
    if (i == 0) {
        return Py_NewRef(Deque_GET_LEFT_ITEM(self));
    }
    if (i == Py_SIZE(self) - 1) {
        return Py_NewRef(Deque_GET_RIGHT_ITEM(self));
    }
    Deque_PHYSICAL_INDEX(self, i, &block_index, &data_index);
    return Py_NewRef(self->blocks[block_index][data_index]);
}

static int
deque_ass_item(deque_t* self, Py_ssize_t i, PyObject* v)
{
    Py_ssize_t block_index, data_index;

    if (!Deque_CHECK_INDEX(i, Py_SIZE(self))) {
        PyErr_SetString(PyExc_IndexError, "deque index out of range");
        return -1;
    }
    Deque_PHYSICAL_INDEX(self, i, &block_index, &data_index);
    Py_SETREF(self->blocks[block_index][data_index], Py_NewRef(v));
    return 0;
}

static block_t
_new_block(deque_t* self)
{
    if (self->num_free_blocks == 0) {
        return PyMem_Malloc(sizeof(PyObject*) * BLOCK_SIZE);
    }
    return self->free_blocks[--self->num_free_blocks];
}

static void
_del_block(deque_t* self, Py_ssize_t block_index)
{
    block_t block = self->blocks[block_index];

    if (self->num_free_blocks >= MAX_FREE_BLOCKS) {
        PyMem_Free(block);
    } else {
        self->free_blocks[self->num_free_blocks++] = block;
    }
}

static int
_update_slots(deque_t* self, Py_ssize_t num_slots)
{
    Py_ssize_t left_block;
    block_t* slots;

    if (num_slots < NUM_INIT_SLOTS) {
        return 0;
    }
    slots = PyMem_Malloc(num_slots * sizeof(block_t));
    if (slots == NULL) {
        return -1;
    }
    left_block = (Py_ssize_t)((num_slots - self->num_blocks) / 2);
    for (Py_ssize_t i = 0; i < self->num_blocks; i++) {
        slots[left_block + i] = self->blocks[self->left_block + i];
    }
    PyMem_Free(self->blocks);
    self->num_slots = num_slots;
    self->blocks = slots;
    self->left_block = left_block;
    self->right_block = left_block + self->num_blocks - 1;
    return 0;
}

static int
_extend_slots(deque_t* self)
{
    Py_ssize_t num_slots = (Py_ssize_t)(self->num_slots * GROWTH_RATE);
    return _update_slots(self, num_slots);
}

static int
_shrink_slots(deque_t* self)
{
    Py_ssize_t num_slots;

    if ((double)(self->num_blocks) / self->num_slots >= SHRINK_RATE / 2.0) {
        return 0;
    }
    num_slots = (Py_ssize_t)(self->num_slots * SHRINK_RATE);
    return _update_slots(self, num_slots);
}

static PyObject*
deque_append(deque_t* self, PyObject* item)
{
    block_t block;

    if (Py_SIZE(self) == 0) {
        block = _new_block(self);
        if (block == NULL) {
            return NULL;
        }
        self->blocks[self->right_block] = block;
        self->right_item--;
        self->num_blocks++;
    } else if (self->right_item == BLOCK_SIZE - 1) {
        if (self->right_block == self->num_slots - 1) {
            if (_extend_slots(self) == -1) {
                return NULL;
            }
        }
        block = _new_block(self);
        if (block == NULL) {
            return NULL;
        }
        self->right_block++;
        self->blocks[self->right_block] = block;
        self->right_item = -1;
        self->num_blocks++;
    }
    self->right_item++;
    Deque_SET_RIGHT_ITEM(self, Py_NewRef(item));
    Py_SET_SIZE(self, Py_SIZE(self) + 1);
    Py_RETURN_NONE;
}

static PyObject*
deque_appendleft(deque_t* self, PyObject* item)
{
    block_t block;

    if (Py_SIZE(self) == 0) {
        block = _new_block(self);
        if (block == NULL) {
            return NULL;
        }
        self->blocks[self->left_block] = block;
        self->left_item++;
        self->num_blocks++;
    } else if (self->left_item == 0) {
        if (self->left_block == 0) {
            if (_extend_slots(self) == -1) {
                return NULL;
            }
        }
        block = _new_block(self);
        if (block == NULL) {
            return NULL;
        }
        self->left_block--;
        self->blocks[self->left_block] = block;
        self->left_item = BLOCK_SIZE;
        self->num_blocks++;
    }
    self->left_item--;
    Deque_SET_LEFT_ITEM(self, Py_NewRef(item));
    Py_SET_SIZE(self, Py_SIZE(self) + 1);
    Py_RETURN_NONE;
}

static PyObject*
_finalize_iterator(PyObject* it)
{
    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_StopIteration)) {
            PyErr_Clear();
        } else {
            Py_DECREF(it);
            return NULL;
        }
    }
    Py_DECREF(it);
    Py_RETURN_NONE;
}

static PyObject*
deque_extend(deque_t* self, PyObject* iterable)
{
    PyObject *it, *item;
    PyObject* (*iternext)(PyObject*);

    if ((PyObject*)self == iterable) {
        PyObject* result;
        PyObject* s = PySequence_List(iterable);
        if (s == NULL) {
            return NULL;
        }
        result = deque_extend(self, s);
        Py_DECREF(s);
        return result;
    }

    it = PyObject_GetIter(iterable);
    if (it == NULL) {
        return NULL;
    }

    iternext = *Py_TYPE(it)->tp_iternext;
    while ((item = iternext(it)) != NULL) {
        if (deque_append(self, item) == NULL) {
            Py_DECREF(item);
            Py_DECREF(it);
            return NULL;
        }
    }
    return _finalize_iterator(it);
}

static PyObject*
deque_extendleft(deque_t* self, PyObject* iterable)
{
    PyObject *it, *item;
    PyObject* (*iternext)(PyObject*);

    if ((PyObject*)self == iterable) {
        PyObject* result;
        PyObject* s = PySequence_List(iterable);
        if (s == NULL) {
            return NULL;
        }
        result = deque_extendleft(self, s);
        Py_DECREF(s);
        return result;
    }

    it = PyObject_GetIter(iterable);
    if (it == NULL) {
        return NULL;
    }

    iternext = *Py_TYPE(it)->tp_iternext;
    while ((item = iternext(it)) != NULL) {
        if (deque_appendleft(self, item) == NULL) {
            Py_DECREF(item);
            Py_DECREF(it);
            return NULL;
        }
    }
    return _finalize_iterator(it);
}

static PyObject*
deque_pop(deque_t* self, PyObject* unused)
{
    PyObject* item;

    if (Py_SIZE(self) == 0) {
        PyErr_SetString(PyExc_IndexError, "pop from an empty deque");
        return NULL;
    }

    item = Deque_GET_RIGHT_ITEM(self);
    Py_SET_SIZE(self, Py_SIZE(self) - 1);

    if (Py_SIZE(self) == 0) {
        _del_block(self, self->right_block);
        self->left_block = self->num_slots >> 1;
        self->right_block = self->left_block;
        self->left_item = BLOCK_SIZE >> 1;
        self->right_item = self->left_item;
        self->num_blocks = 0;
    } else if (self->right_item == 0) {
        _del_block(self, self->right_block);
        self->num_blocks--;
        self->right_block--;
        self->right_item = BLOCK_SIZE - 1;
    } else {
        self->right_item--;
    }
    if (_shrink_slots(self) == -1) {
        Py_DECREF(item);
        return NULL;
    }
    return item;
}

static PyObject*
deque_popleft(deque_t* self, PyObject* unused)
{
    PyObject* item;

    if (Py_SIZE(self) == 0) {
        PyErr_SetString(PyExc_IndexError, "pop from an empty deque");
        return NULL;
    }

    item = Deque_GET_LEFT_ITEM(self);
    Py_SET_SIZE(self, Py_SIZE(self) - 1);

    if (Py_SIZE(self) == 0) {
        _del_block(self, self->left_block);
        self->left_block = self->num_slots >> 1;
        self->right_block = self->left_block;
        self->left_item = BLOCK_SIZE >> 1;
        self->right_item = self->left_item;
        self->num_blocks = 0;
    } else if (self->left_item == BLOCK_SIZE - 1) {
        _del_block(self, self->left_block);
        self->num_blocks--;
        self->left_block++;
        self->left_item = 0;
    } else {
        self->left_item++;
    }
    if (_shrink_slots(self) == -1) {
        Py_DECREF(item);
        return NULL;
    }
    return item;
}

static Py_ssize_t
deque_length(deque_t* self)
{
    return Py_SIZE(self);
}

static PyObject*
deque_get_maxlen(deque_t* self, void* Py_UNUSED(ignored))
{
    if (self->maxlen < 0) {
        Py_RETURN_NONE;
    }
    return PyLong_FromSsize_t(self->maxlen);
}

static int
deque_traverse(deque_t* self, visitproc visit, void* arg)
{
    Py_ssize_t block_index, data_index;

    Py_VISIT(Py_TYPE(self));
    for (Py_ssize_t i = 0; i < Py_SIZE(self); i++) {
        Deque_PHYSICAL_INDEX(self, i, &block_index, &data_index);
        Py_VISIT(self->blocks[block_index][data_index]);
    }
    return 0;
}

static int
deque_clear(deque_t* self)
{
    block_t block;
    Py_ssize_t i, j;

    if (Py_SIZE(self) == 0) {
        return 0;
    }

    if (self->num_blocks == 1) {
        block = self->blocks[self->left_block];
        for (j = self->left_item; j <= self->right_item; j++) {
            Py_DECREF(block[j]);
        }
    }

    if (self->num_blocks >= 2) {
        block = self->blocks[self->left_block];
        for (j = self->left_item; j < BLOCK_SIZE; j++) {
            Py_DECREF(block[j]);
        }
        block = self->blocks[self->right_block];
        for (j = 0; j <= self->right_item; j++) {
            Py_DECREF(block[j]);
        }
    }

    if (self->num_blocks > 2) {
        for (i = (self->left_block + 1); i < self->right_block; i++) {
            block = self->blocks[i];
            for (j = 0; j < BLOCK_SIZE; j++) {
                Py_DECREF(block[j]);
            }
        }
    }

    for (i = self->left_block; i <= self->right_block; i++) {
        _del_block(self, i);
    }
    PyMem_Free(self->blocks);
    self->blocks = PyMem_Malloc(NUM_INIT_SLOTS * sizeof(block_t));
    if (self->blocks == NULL) {
        return -1;
    }

    Py_SET_SIZE(self, 0);
    self->left_block = NUM_INIT_SLOTS >> 1;
    self->right_block = self->left_block;
    self->left_item = BLOCK_SIZE >> 1;
    self->right_item = self->left_item;
    self->num_blocks = 0;
    self->num_slots = NUM_INIT_SLOTS;
    return 0;
}

static PyObject*
deque_clearmethod(deque_t* self, PyObject* Py_UNUSED(ignored))
{
    if (deque_clear(self) == -1) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
deque_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
    deque_t* self = (deque_t*)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    self->blocks = PyMem_Malloc(NUM_INIT_SLOTS * sizeof(block_t));
    if (self->blocks == NULL) {
        return NULL;
    }
    self->left_block = NUM_INIT_SLOTS >> 1;
    self->right_block = self->left_block;
    self->left_item = BLOCK_SIZE >> 1;
    self->right_item = self->left_item;
    self->maxlen = -1;
    self->num_slots = NUM_INIT_SLOTS;
    self->num_blocks = 0;
    self->num_free_blocks = 0;
    self->weakreflist = NULL;
    Py_SET_SIZE(self, 0);
    return (PyObject*)self;
}

static int
deque_init(deque_t* self, PyObject* args, PyObject* kwdargs)
{
    PyObject* iterable = NULL;
    PyObject* maxlenobj = NULL;
    Py_ssize_t maxlen = -1;
    char* kwlist[] = { "iterable", "maxlen", 0 };

    if (kwdargs == NULL && PyTuple_GET_SIZE(args) <= 2) {
        if (PyTuple_GET_SIZE(args) > 0) {
            iterable = PyTuple_GET_ITEM(args, 0);
        }
        if (PyTuple_GET_SIZE(args) > 1) {
            goto maxlen_error;
        }
    } else {
        if (!PyArg_ParseTupleAndKeywords(
                args, kwdargs, "|OO:deque", kwlist, &iterable, &maxlenobj))
            return -1;
    }
    if (maxlenobj != NULL) {
        goto maxlen_error;
    }
    if (Py_SIZE(self) > 0) {
        if (deque_clear(self) == -1) {
            return -1;
        }
    }
    if (iterable != NULL) {
        PyObject* rv = deque_extend(self, iterable);
        if (rv == NULL) {
            return -1;
        }
        Py_DECREF(rv);
    }
    return 0;

maxlen_error:
    PyErr_SetString(PyExc_ValueError, "maxlen argument is not yet supported");
    return -1;
}

static void
deque_dealloc(deque_t* self)
{
    PyObject_GC_UnTrack(self);
    if (self->weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject*)self);
    }
    deque_clear(self);
    if (self->num_blocks > 0) {
        for (Py_ssize_t i = self->left_block; i <= self->right_block; i++) {
            PyMem_FREE(self->blocks[i]);
        }
    }
    PyMem_Free(self->blocks);
    for (Py_ssize_t i = 0; i < self->num_free_blocks; i++) {
        PyMem_FREE(self->free_blocks[i]);
    }
    Py_TYPE(self)->tp_free(self);
}

static PyObject*
deque_repr(PyObject* self)
{
    PyObject *aslist, *result;

    int i = Py_ReprEnter(self);
    if (i != 0) {
        if (i < 0) {
            return NULL;
        }
        return PyUnicode_FromString("[...]");
    }

    aslist = PySequence_List(self);
    if (aslist == NULL) {
        Py_ReprLeave(self);
        return NULL;
    }
    if (((deque_t*)self)->maxlen >= 0)
        result = PyUnicode_FromFormat("%s(%R, maxlen=%zd)",
            _PyType_Name(Py_TYPE(self)), aslist, ((deque_t*)self)->maxlen);
    else
        result = PyUnicode_FromFormat("%s(%R)",
            _PyType_Name(Py_TYPE(self)), aslist);
    Py_ReprLeave(self);
    Py_DECREF(aslist);
    return result;
}

static PyObject*
deque_sizeof(deque_t* self, void* unused)
{
    size_t res = _PyObject_SIZE(Py_TYPE(self));
    res += self->num_slots * sizeof(block_t);
    res += (self->num_blocks + self->num_free_blocks) * BLOCK_SIZE * sizeof(PyObject*);
    return PyLong_FromSize_t(res);
}

//
// deque object
//

static PyGetSetDef deque_getset[] = {
    { "maxlen", (getter)deque_get_maxlen, (setter)NULL, NULL },
    { 0 },
};

static PyMethodDef deque_methods[] = {
    { "append", (PyCFunction)deque_append, METH_O, NULL },
    { "appendleft", (PyCFunction)deque_appendleft, METH_O, NULL },
    { "clear", (PyCFunction)deque_clearmethod, METH_NOARGS, NULL },
    { "pop", (PyCFunction)deque_pop, METH_NOARGS, NULL },
    { "popleft", (PyCFunction)deque_popleft, METH_NOARGS, NULL },
    { "extend", (PyCFunction)deque_extend, METH_O, NULL },
    { "extendleft", (PyCFunction)deque_extendleft, METH_O, NULL },
    { "__sizeof__", (PyCFunction)deque_sizeof, METH_NOARGS, NULL },
    { "__class_getitem__", Py_GenericAlias, METH_O | METH_CLASS, PyDoc_STR("See PEP 585") },
    { NULL, NULL }, // sentinel
};

static PyMemberDef deque_members[] = {
    { "__weaklistoffset__", T_PYSSIZET, offsetof(deque_t, weakreflist), READONLY },
    { NULL },
};

static PySequenceMethods deque_as_sequence = {
    .sq_length = (lenfunc)deque_length,
    .sq_item = (ssizeargfunc)deque_item,
    .sq_ass_item = (ssizeargfunc)deque_ass_item,
};

static PyTypeObject deque_type = {
    .tp_name = "deque.deque",
    .tp_basicsize = sizeof(deque_t),
    .tp_dealloc = deque_dealloc,
    .tp_as_sequence = &deque_as_sequence,
    .tp_getset = &deque_getset,
    .tp_flags = Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_HAVE_GC
        | Py_TPFLAGS_SEQUENCE,
    .tp_traverse = deque_traverse,
    .tp_clear = deque_clear,
    .tp_repr = deque_repr,
    .tp_hash = PyObject_HashNotImplemented,
    .tp_weaklistoffset = offsetof(deque_t, weakreflist),
    .tp_methods = deque_methods,
    .tp_new = deque_new,
    .tp_init = deque_init,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
};

//
// deque module
//

static PyModuleDef deque_module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "deque",
};

PyMODINIT_FUNC
PyInit_deque(void)
{
    if (PyType_Ready(&deque_type) < 0) {
        return NULL;
    }

    PyObject* m = PyModule_Create(&deque_module);
    if (m == NULL) {
        return NULL;
    }

    Py_INCREF(&deque_type);
    PyModule_AddObject(m, "deque", (PyObject*)&deque_type);
    return m;
}
