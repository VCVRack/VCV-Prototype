extern "C" {
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
}
#include <dlfcn.h>
#include "ScriptEngine.hpp"
#include <thread>


extern rack::Plugin* pluginInstance;


static void initPython() {
	if (Py_IsInitialized())
		return;

	// Okay, this is an IQ 200 solution for fixing the following issue.
	// - Rack (the "application") `dlopen()`s this plugin with RTLD_LOCAL.
	// - This plugin links with libpython, either statically or dynamically. In either case, symbols are hidden to "outside" libraries.
	// - A Python script runs `import math` for example, which loads math.cpython*.so.
	// - Since that's an "outside" library, it can't access libpython symbols, because it doesn't link to libpython itself.
	// The best solution I have is to dlopen() with RTLD_GLOBAL within the plugin, which will make all libpython symbols available to the *entire* Rack application.
	// The plugin still needs to -lpython because otherwise Rack will complain that there are unresolved symbols in the plugin, so after the following lines, libpython will be in memory *twice*, unless dlopen() is doing some optimization I'm not aware of.
	std::string libDir = rack::asset::plugin(pluginInstance, "dep/lib");
	std::string pythonLib = libDir + "/libpython3.7m.so";
	void* handle = dlopen(pythonLib.c_str(), RTLD_NOW | RTLD_GLOBAL);
	assert(handle);
	// Set python path
	std::string sep = ":";
	std::string pythonPath = libDir + "/python3.7";
	pythonPath += sep + libDir + "/python3.7/lib-dynload";
	wchar_t* pythonPathW = Py_DecodeLocale(pythonPath.c_str(), NULL);
	Py_SetPath(pythonPathW);
	PyMem_RawFree(pythonPathW);
	// Initialize but don't register signal handlers
	Py_InitializeEx(0);

	// PyEval_InitThreads();
}


struct PythonEngine : ScriptEngine {
	PyObject* mainDict = NULL;
	PyObject* processFunc = NULL;
	PyObject* blockObj = NULL;
	PyInterpreterState* interp = NULL;

	~PythonEngine() {
		if (interp)
			PyInterpreterState_Delete(interp);
	}

	std::string getEngineName() override {
		return "Python";
	}

	int run(const std::string& path, const std::string& script) override {
		initPython();

		// PyThreadState* tstate = PyThreadState_Get();
		// interp = PyInterpreterState_New();
		// PyThreadState_Swap(tstate);

		// Get globals dictionary
		PyObject* mainModule = PyImport_AddModule("__main__");
		assert(mainModule);
		mainDict = PyModule_GetDict(mainModule);
		assert(mainDict);

		// Add functions to globals
		DEFER({Py_DECREF(mainDict);});
		static PyMethodDef native_functions[] = {
			{"display", native_display, METH_VARARGS, ""},
			{NULL, NULL, 0, NULL},
		};
		if (PyModule_AddFunctions(mainModule, native_functions)) {
			WARN("Could not add global functions");
			return -1;
		}

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

		// Get process function from globals
		processFunc = PyDict_GetItemString(mainDict, "process");
		if (!processFunc) {
			setMessage("No process() function");
			return -1;
		}
		if (!PyCallable_Check(processFunc)) {
			setMessage("process() is not callable");
			return -1;
		}

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
		static PyStructSequence_Desc blockDesc = {"block", "", blockFields, LENGTHOF(blockFields) - 1};
		PyTypeObject* blockType = PyStructSequence_NewType(&blockDesc);
		assert(blockType);
		blockObj = PyStructSequence_New(blockType);
		assert(blockObj);
		PyStructSequence_SetItem(blockObj, 1, PyFloat_FromDouble(42.f));
		PyStructSequence_SetItem(blockObj, 2, PyFloat_FromDouble(42.f));
		PyStructSequence_SetItem(blockObj, 3, PyFloat_FromDouble(42.f));
		PyStructSequence_SetItem(blockObj, 4, PyFloat_FromDouble(42.f));
		PyStructSequence_SetItem(blockObj, 5, PyFloat_FromDouble(42.f));

		// inputs
		// npy_intp dims[] = {NUM_ROWS, MAX_BUFFER_SIZE};
		// PyObject* inputs = PyArray_SimpleNewFromData(2, dims, NPY_FLOAT32, block->inputs);
		// PyStructSequence_SetItem(blockObj, 0, inputs);

		return 0;
	}

	int process() override {
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

			// setMessage(str);
			return -1;
		}
		DEFER({Py_DECREF(processResult);});

		// PyThreadState* tstate = PyThreadState_New(interp);
		// PyEval_RestoreThread(tstate);
		// PyThreadState_Clear(tstate);
		// PyThreadState_DeleteCurrent();
		return 0;
	}

	static PyObject* native_display(PyObject* self, PyObject* args) {
		PyObject* msg = PyTuple_GetItem(args, 0);
		if (!msg)
			return NULL;

		const char* msgS = PyUnicode_AsUTF8(msg);
		DEBUG("%s", msgS);

		Py_INCREF(Py_None);
		return Py_None;
	}
};


ScriptEngine* createPythonEngine() {
	return new PythonEngine;
}