#include "ScriptEngine.hpp"
#include <Python.h>


extern rack::Plugin* pluginInstance;


struct PythonEngine : ScriptEngine {
	PyInterpreterState* interp = NULL;

	~PythonEngine() {
		if (interp)
			PyInterpreterState_Delete(interp);
	}

	std::string getEngineName() override {
		return "Python";
	}

	int run(const std::string& path, const std::string& script) override {
		// Initialize Python
		if (!Py_IsInitialized()) {
			std::string pythonPath = rack::asset::plugin(pluginInstance, "dep/lib/python3.7");
			wchar_t* pythonPathW = Py_DecodeLocale(pythonPath.c_str(), NULL);
			Py_SetPath(pythonPathW);
			PyMem_RawFree(pythonPathW);
			Py_InitializeEx(0);

			PyEval_InitThreads();
		}
		if (!Py_IsInitialized()) {
			return -1;
		}

		PyThreadState* tstate = PyThreadState_Get();
		interp = PyInterpreterState_New();
		PyThreadState_Swap(tstate);

		if (PyRun_SimpleString(script.c_str())) {
			return -1;
		}

		// rack::DEBUG("4");
		return 0;
	}

	int process() override {
		PyThreadState* tstate = PyThreadState_New(interp);
		PyEval_RestoreThread(tstate);
		PyThreadState_Clear(tstate);
		PyThreadState_DeleteCurrent();
		return 0;
	}
};


ScriptEngine* createPythonEngine() {
	return new PythonEngine;
}