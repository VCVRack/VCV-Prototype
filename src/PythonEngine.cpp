extern "C" {
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
}
#include <dlfcn.h>
#include "ScriptEngine.hpp"
#include <thread>


/*
TODO:
- Fix Numpy loading on Linux.
	- "undefined symbol: PyExc_RecursionError"
	- This worked in Python 3.7, but because some other crash (regarding GIL interpreter objects, I forgot) was fixed on Python's issue tracker in the last month, I switched to Python 3.8 to solve that problem.
- Test build and running on Windows/Mac. (Has not been attempted.)
- Allow multiple instances with GIL.
- Fix destructors.
*/


extern rack::Plugin* pluginInstance;


static void initPython() {
	if (Py_IsInitialized())
		return;

	std::string pythonDir = rack::asset::plugin(pluginInstance, "dep/lib/python3.8");
	// Set python path
	std::string sep = ":";
	std::string pythonPath = pythonDir;
	pythonPath += sep + pythonDir + "/lib-dynload";
	pythonPath += sep + pythonDir + "/site-packages";
	wchar_t* pythonPathW = Py_DecodeLocale(pythonPath.c_str(), NULL);
	Py_SetPath(pythonPathW);
	PyMem_RawFree(pythonPathW);
	// Initialize but don't register signal handlers
	Py_InitializeEx(0);
	assert(Py_IsInitialized());

	// PyEval_InitThreads();

	// Import numpy
	if (_import_array()) {
		PyErr_Print();
		abort();
	}
}


struct PythonEngine : ScriptEngine {
	PyObject* mainDict = NULL;
	PyObject* processFunc = NULL;
	PyObject* blockObj = NULL;
	PyInterpreterState* interp = NULL;

	~PythonEngine() {
		if (mainDict)
			Py_DECREF(mainDict);
		if (processFunc)
			Py_DECREF(processFunc);
		if (blockObj)
			Py_DECREF(blockObj);
		if (interp)
			PyInterpreterState_Delete(interp);
	}

	std::string getEngineName() override {
		return "Python";
	}

	int run(const std::string& path, const std::string& script) override {
		ProcessBlock* block = getProcessBlock();
		initPython();

		// PyThreadState* tstate = PyThreadState_Get();
		// interp = PyInterpreterState_New();
		// PyThreadState_Swap(tstate);

		// Get globals dictionary
		PyObject* mainModule = PyImport_AddModule("__main__");
		assert(mainModule);
		DEFER({Py_DECREF(mainModule);});
		mainDict = PyModule_GetDict(mainModule);
		assert(mainDict);

		// Set context pointer
		PyObject* engineObj = PyCapsule_New(this, NULL, NULL);
		PyDict_SetItemString(mainDict, "_engine", engineObj);

		// Add functions to globals
		static PyMethodDef native_functions[] = {
			{"display", nativeDisplay, METH_VARARGS, ""},
			{NULL, NULL, 0, NULL},
		};
		if (PyModule_AddFunctions(mainModule, native_functions)) {
			WARN("Could not add global functions");
			return -1;
		}

		// Set config
		static PyStructSequence_Field configFields[] = {
			{"frameDivider", ""},
			{"bufferSize", ""},
			{NULL, NULL},
		};
		static PyStructSequence_Desc configDesc = {"Config", "", configFields, LENGTHOF(configFields) - 1};
		PyTypeObject* configType = PyStructSequence_NewType(&configDesc);
		assert(configType);

		PyObject* configObj = PyStructSequence_New(configType);
		assert(configObj);
		PyDict_SetItemString(mainDict, "config", configObj);

		// frameDivider
		PyStructSequence_SetItem(configObj, 0, PyLong_FromLong(32));
		// bufferSize
		PyStructSequence_SetItem(configObj, 1, PyLong_FromLong(1));

		// Compile string
		PyObject* code = Py_CompileString(script.c_str(), path.c_str(), Py_file_input);
		if (!code) {
			PyErr_Print();
			return -1;
		}
		DEFER({Py_DECREF(code);});

		// Evaluate string
		PyObject* result = PyEval_EvalCode(code, mainDict, mainDict);
		if (!result) {
			PyErr_Print();
			return -1;
		}
		DEFER({Py_DECREF(result);});

		// Create block
		static PyStructSequence_Field blockFields[] = {
			{"inputs", ""},
			{"outputs", ""},
			{"knobs", ""},
			{"switches", ""},
			{"lights", ""},
			{"switch_lights", ""},
			{NULL, NULL},
		};
		static PyStructSequence_Desc blockDesc = {"Block", "", blockFields, LENGTHOF(blockFields) - 1};
		PyTypeObject* blockType = PyStructSequence_NewType(&blockDesc);
		assert(blockType);
		DEBUG("ref %d", Py_REFCNT(blockType));

		blockObj = PyStructSequence_New(blockType);
		assert(blockObj);
		DEBUG("ref %d", Py_REFCNT(blockObj));

		// inputs
		npy_intp inputsDims[] = {NUM_ROWS, MAX_BUFFER_SIZE};
		PyObject* inputs = PyArray_SimpleNewFromData(2, inputsDims, NPY_FLOAT32, block->inputs);
		PyStructSequence_SetItem(blockObj, 0, inputs);

		// outputs
		npy_intp outputsDims[] = {NUM_ROWS, MAX_BUFFER_SIZE};
		PyObject* outputs = PyArray_SimpleNewFromData(2, outputsDims, NPY_FLOAT32, block->outputs);
		PyStructSequence_SetItem(blockObj, 1, outputs);

		// knobs
		npy_intp knobsDims[] = {NUM_ROWS};
		PyObject* knobs = PyArray_SimpleNewFromData(1, knobsDims, NPY_FLOAT32, block->knobs);
		PyStructSequence_SetItem(blockObj, 2, knobs);

		// switches
		npy_intp switchesDims[] = {NUM_ROWS};
		PyObject* switches = PyArray_SimpleNewFromData(1, switchesDims, NPY_BOOL, block->switches);
		PyStructSequence_SetItem(blockObj, 3, switches);

		// lights
		npy_intp lightsDims[] = {NUM_ROWS, 3};
		PyObject* lights = PyArray_SimpleNewFromData(2, lightsDims, NPY_FLOAT32, block->lights);
		PyStructSequence_SetItem(blockObj, 4, lights);

		// switchLights
		npy_intp switchLightsDims[] = {NUM_ROWS, 3};
		PyObject* switchLights = PyArray_SimpleNewFromData(2, switchLightsDims, NPY_FLOAT32, block->switchLights);
		PyStructSequence_SetItem(blockObj, 5, switchLights);

		// Get process function from globals
		processFunc = PyDict_GetItemString(mainDict, "process");
		if (!processFunc) {
			display("No process() function");
			return -1;
		}
		if (!PyCallable_Check(processFunc)) {
			display("process() is not callable");
			return -1;
		}

		return 0;
	}

	int process() override {
		// DEBUG("ref %d", Py_REFCNT(blockObj));
		// Call process()
		PyObject* args = PyTuple_Pack(1, blockObj);
		assert(args);
		DEFER({Py_DECREF(args);});
		PyObject* processResult = PyObject_CallObject(processFunc, args);
		if (!processResult) {
			PyErr_Print();

			// PyObject *ptype, *pvalue, *ptraceback;
			// PyErr_Fetch(&ptype, &pvalue, &ptraceback);
			// const char* str = PyUnicode_AsUTF8(pvalue);
			// if (!str)
			// 	return -1;

			// display(str);
			return -1;
		}
		DEFER({Py_DECREF(processResult);});

		// PyThreadState* tstate = PyThreadState_New(interp);
		// PyEval_RestoreThread(tstate);
		// PyThreadState_Clear(tstate);
		// PyThreadState_DeleteCurrent();
		return 0;
	}

	static PyObject* nativeDisplay(PyObject* self, PyObject* args) {
		PyObject* mainDict = PyEval_GetGlobals();
		assert(mainDict);
		PyObject* engineObj = PyDict_GetItemString(mainDict, "_engine");
		assert(engineObj);
		PythonEngine* engine = (PythonEngine*) PyCapsule_GetPointer(engineObj, NULL);
		assert(engine);

		PyObject* msgO = PyTuple_GetItem(args, 0);
		if (!msgO)
			return NULL;

		PyObject* msgS = PyObject_Str(msgO);
		DEFER({Py_DECREF(msgS);});

		const char* msg = PyUnicode_AsUTF8(msgS);
		engine->display(msg);

		Py_INCREF(Py_None);
		return Py_None;
	}
};


__attribute__((constructor(1000)))
static void constructor() {
	addScriptEngine<PythonEngine>(".py");
}
