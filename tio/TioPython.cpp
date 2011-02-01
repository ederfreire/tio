

#include "pch.h"
#include "TioPython.h"
#include "ContainerManager.h"

namespace tio
{
	using std::vector;
	using std::string;
	using std::cout;
	using std::endl;
	using boost::shared_ptr;
	namespace python = boost::python;

	python::object g_pythonContainerManager;

	template<typename T, typename TDerived>
	class PythonWrapperImpl
	{
		typedef PythonWrapperImpl<T, TDerived> this_type;

		void SetWrapped(T wrapped)
		{
			wrapped_ = wrapped;
		}

		static void InitWrapperT()
		{
			if(pythonWrapperT.is_none())
				pythonWrapperT = TDerived::InitPythonType();
		}

	protected:
		T wrapped_;
		static python::object pythonWrapperT;
	public:

		static python::object CreateWrapper(T wrapped)
		{
			InitWrapperT();

			//
			// create the wrapper as Python object
			//
			python::object pythonWrapper = pythonWrapperT();

			//
			// extract the C++ object
			//
			TDerived& wrapper = python::extract<TDerived&>(pythonWrapper);

			//
			// set the wrapped C++ object
			//
			wrapper.SetWrapped(wrapped);
			
			return pythonWrapper;
		}
	};

	template<typename T, typename TDerived> python::object PythonWrapperImpl<T, TDerived>::pythonWrapperT;

	
//#define INITIALIZE_PYTHON_WRAPPER(T) python::object T::pythonWrapperT;

	TioData PythonObjectToTioData(const python::object& obj)
	{
		if(PyString_Check(obj.ptr()))
			return TioData(python::extract<string>(obj));
		else if(PyInt_Check(obj.ptr()))
			return TioData(python::extract<int>(obj));
		else if(PyFloat_Check(obj.ptr()))
			return TioData(python::extract<float>(obj));

		throw std::runtime_error("invalid python type");
	}

	python::object TioDataToPythonObject(const TioData& v)
	{
		switch(v.GetDataType())
		{
		case TioData::Sz:
			return python::object(v.AsSz());
		case TioData::Int:
			return python::object(v.AsInt());
		case TioData::Double:
			return python::object(v.AsDouble());
		}

		return python::object();
	}

	class TioResultSetWrapper : public PythonWrapperImpl<shared_ptr<ITioResultSet> , TioResultSetWrapper>
	{
	public:
		static python::object InitPythonType()
		{
				return 
					python::class_<TioResultSetWrapper>("ResultSet")
						.def("get", &TioResultSetWrapper::GetRecord)
						.def("next", &TioResultSetWrapper::MoveNext)
						.def("previous", &TioResultSetWrapper::MovePrevious)
						.def("at_begin", &TioResultSetWrapper::AtBegin)
						.def("at_end", &TioResultSetWrapper::AtEnd)
						.def("count", &TioResultSetWrapper::RecordCount);
		}

		void MoveNext() { wrapped_->MoveNext(); }
		void MovePrevious() { wrapped_->MovePrevious(); }
		bool AtBegin() { return wrapped_->AtBegin(); }
		bool AtEnd() { return wrapped_->AtEnd(); }
		unsigned int RecordCount() { return wrapped_->RecordCount();}

		python::object Source() 
		{ 
			return TioDataToPythonObject(wrapped_->Source());
		}

		python::object GetRecord()
		{
			TioData key, value, metadata;
			
			wrapped_->GetRecord(&key, &value, &metadata);

			return python::make_tuple(
				TioDataToPythonObject(key),
				TioDataToPythonObject(value),
				TioDataToPythonObject(metadata));
		}
	};

	class TioContainerWrapper : public PythonWrapperImpl< shared_ptr<ITioContainer>, TioContainerWrapper>
	{
	public:

		static python::object InitPythonType()
		{
			using python::arg;

			string containerPythonizer =  
				"class ContainerPythonizer(object):\r\n"
				"    def __del__(self):\r\n"
				"        self.Close()\r\n"
				"    def __getitem__(self, key):\r\n"
				"        if isinstance(key, slice):\r\n"
				"            return self.query(key.start, key.stop)\r\n"
				"        else:\r\n"
				"            return self.get(key)\r\n"
				"    def __delitem__(self, key):\r\n"
				"        return self.delete(key)    \r\n"
				"    def __len__(self):\r\n"
				"        return self.get_count()    \r\n"
				"    def __setitem__(self, key, valueOrValueAndMetadata):\r\n"
				"        if isinstance(valueOrValueAndMetadata, tuple):\r\n"
				"            value, metadata = valueOrValueAndMetadata\r\n"
				"        else:\r\n"
				"            value, metadata = valueOrValueAndMetadata, None\r\n"
				"        return self.set(key, value, metadata)\r\n"
				"    def append(self, value, metadata=None):\r\n"
				"        return self.push_back(value, metadata)\r\n"
				"    def e\r\ntend(self, iterable):\r\n"
				"        for \r\n in iterable:\r\n"
				"            self.push_back(\r\n)\r\n"
				"    def values(self):\r\n"
				"        return self.query()    \r\n"
				"    def keys(self):\r\n"
				"        return [\r\n[0] for \r\n in self.query_with_key_and_metadata()]\r\n";

			python::object containerBase = 
				python::class_<TioContainerWrapper>("ContainerBase")
					.def("push_back", &TioContainerWrapper::PushBack)
					.def("append", &TioContainerWrapper::PushBackValue)
					.def("pop_back", &TioContainerWrapper::PopBack)
					.def("push_front", &TioContainerWrapper::PushFront)
					.def("pop_front", &TioContainerWrapper::PopFront)
					.def("insert", &TioContainerWrapper::Insert)
					.def("set", &TioContainerWrapper::Set, (arg("key"), arg("value"), arg("metadata")))
					.def("get", &TioContainerWrapper::GetRecord)
					.def("delete", &TioContainerWrapper::Delete)
					.def("clear", &TioContainerWrapper::Clear)
					.def("set_property", &TioContainerWrapper::SetProperty)
					.def("get_property", &TioContainerWrapper::GetProperty)
					.def("subscribe", &TioContainerWrapper::Subscribe)
					.def("subscribe", &TioContainerWrapper::Subscribe1)
					.def("unsubscribe", &TioContainerWrapper::Unsubscribe)
					.def("__len__", &TioContainerWrapper::GetRecordCount)
					.def("__getitem__", &TioContainerWrapper::GetRecordValue)
					.def("__setitem__", &TioContainerWrapper::Set1)
					//.def("__delitem__", &map_item<Key,Val>().del)
					.add_property("name", &TioContainerWrapper::GetName);

			python::object main_module((python::handle<>(python::borrowed(PyImport_AddModule("__main__")))));
			python::object main_namespace = main_module.attr("__dict__");

			main_namespace["ContainerBase"] = containerBase;
			PyRun_String(containerPythonizer.c_str(), 0, main_namespace.ptr(), main_namespace.ptr());

			return main_namespace["Container"];

		}

		static void PythonCallbackBridge(python::object container, python::object callback, const string& eventFilter, const string& eventName, 
			const TioData& key, const TioData& value, const TioData& metadata)
		{
			if(eventFilter.empty() == false && eventFilter != eventName)
				return;

			callback(
				container,
				eventName,
				TioDataToPythonObject(key),
				TioDataToPythonObject(value),
				TioDataToPythonObject(metadata));
		}

		int Subscribe(python::object callback, const string& eventFilter, python::object start)
		{
			return wrapped_->Subscribe(
				boost::bind(&TioContainerWrapper::PythonCallbackBridge, python::object(this), callback, eventFilter == "*" ? string() : eventFilter, _1, _2, _3, _4),
				"0");
		}

		int Subscribe1(python::object callback)
		{
			return Subscribe(callback, string(), python::object());
		}

		void Unsubscribe(unsigned int cookie)
		{
			wrapped_->Unsubscribe(cookie);
		}

		void Clear()
		{
			wrapped_->Clear();
		}

		string GetProperty(const string& key)
		{
			return wrapped_->GetProperty(key);
		}

		void SetProperty(const string& key, const string& value)
		{
			return wrapped_->SetProperty(key, value);
		}

		int GetRecordCount()
		{
			return wrapped_->GetRecordCount();
		}

		python::object GetRecord(python::object searchKey)
		{
			TioData key, value, metadata;
			
			wrapped_->GetRecord(PythonObjectToTioData(searchKey), &key, &value, &metadata);

			return python::make_tuple(
				TioDataToPythonObject(key),
				TioDataToPythonObject(value),
				TioDataToPythonObject(metadata));
		}

		python::object GetRecordValue(python::object searchKey)
		{
			TioData key, value;
			
			wrapped_->GetRecord(PythonObjectToTioData(searchKey), &key, &value, NULL);

			return TioDataToPythonObject(value);
		}

		python::object PopBack(python::object searchKey)
		{
			TioData key, value, metadata;
			
			wrapped_->PopBack(&key, &value, &metadata);

			return python::make_tuple(
				TioDataToPythonObject(key),
				TioDataToPythonObject(value),
				TioDataToPythonObject(metadata));
		}

		python::object PopFront(python::object searchKey)
		{
			TioData key, value, metadata;
			
			wrapped_->PopFront(&key, &value, &metadata);

			return python::make_tuple(
				TioDataToPythonObject(key),
				TioDataToPythonObject(value),
				TioDataToPythonObject(metadata));
		}

		void PushFront(python::object key, python::object value, python::object metadata)
		{
			wrapped_->PushFront(
				PythonObjectToTioData(key),
				PythonObjectToTioData(value),
				PythonObjectToTioData(metadata));
		}

		void PushBack(python::object key, python::object value, python::object metadata)
		{
			wrapped_->PushBack(
				PythonObjectToTioData(key),
				PythonObjectToTioData(value),
				PythonObjectToTioData(metadata));
		}

		void PushBackValue(python::object value)
		{
			wrapped_->PushBack(TIONULL, PythonObjectToTioData(value));
		}

		void Insert(python::object key, python::object value, python::object metadata)
		{
			wrapped_->Insert(
				PythonObjectToTioData(key),
				PythonObjectToTioData(value),
				PythonObjectToTioData(metadata));
		}

		void Set(python::object key, python::object value, python::object metadata)
		{
			wrapped_->Set(
				PythonObjectToTioData(key),
				PythonObjectToTioData(value),
				PythonObjectToTioData(metadata));
		}

		void Set1(python::object key, python::object value)
		{
			wrapped_->Set(
				PythonObjectToTioData(key),
				PythonObjectToTioData(value),
				TIONULL);
		}

		void Delete(python::object key, python::object value, python::object metadata)
		{
			wrapped_->Delete(
				PythonObjectToTioData(key),
				PythonObjectToTioData(value),
				PythonObjectToTioData(metadata));
		}

		string GetName()
		{
			return wrapped_->GetName();
		}
	};

	class TioContainerManagerWrapper : public PythonWrapperImpl<ContainerManager* , TioContainerManagerWrapper>
	{
	public:
		static python::object InitPythonType()
		{
				return 
					python::class_<TioContainerManagerWrapper>("ContainerManager")
						.def("create", &TioContainerManagerWrapper::CreateContainer)
						.def("open", &TioContainerManagerWrapper::OpenContainer1)
						.def("open", &TioContainerManagerWrapper::OpenContainer2)
						.def("delete", &TioContainerManagerWrapper::DeleteContainer)
						.def("exists", &TioContainerManagerWrapper::Exists);

		}

		python::object CreateContainer(string name, string type)
		{
			return TioContainerWrapper::CreateWrapper(
				wrapped_->CreateContainer(type, name));
		}

		python::object OpenContainer2(string name, string type)
		{
			return TioContainerWrapper::CreateWrapper(
				wrapped_->OpenContainer(type, name));
		}

		python::object OpenContainer1(string name)
		{
			return TioContainerWrapper::CreateWrapper(
				wrapped_->OpenContainer(string(), name));
		}

		void DeleteContainer(const string& name, const string& type)
		{
			wrapped_->DeleteContainer(type, name);
		}

		bool Exists(string name, string type)
		{
			return wrapped_->Exists(type, name);
		}
	};

//	INITIALIZE_PYTHON_WRAPPER(TioContainerManagerWrapper);
//	INITIALIZE_PYTHON_WRAPPER(TioContainerWrapper);

	void InitializePythonSupport(const char* programName, ContainerManager* containerManager)
	{
		Py_SetProgramName(const_cast<char*>(programName));
		Py_Initialize();
		PyRun_SimpleString("print 'Python support initialized:'");

		/*
		//
		// Show Python version
		//
		python::object sys = boost::python::import("sys");
		string version = boost::python::extract<std::string>(sys.attr("version"));
		cout << version << endl;

		python::object main_module((
			python::handle<>(python::borrowed(PyImport_AddModule("__main__")))));

		python::object main_namespace = main_module.attr("__dict__");
		*/

		try
		{
			g_pythonContainerManager = TioContainerManagerWrapper::CreateWrapper(containerManager);
		}
		catch(boost::python::error_already_set&)
		{
			PyErr_Print();
			throw std::runtime_error("Error loading python infrastructure ");
		}
	}

	void LoadPythonPlugins(const vector<string>& plugins)
	{
		boost::python::object imp = boost::python::import("imp");
		boost::python::object load_source = imp.attr("load_source");
	
		BOOST_FOREACH(const string& pluginPath, plugins)
		{
			try
			{
				boost::python::object pluginModule = load_source(boost::filesystem::path(pluginPath).stem(), pluginPath);
				pluginModule.attr("TioPluginMain")(g_pythonContainerManager);
			}
			catch(boost::python::error_already_set&)
			{
				PyErr_Print();
				throw std::runtime_error(string("Error loading python plugin ") + pluginPath);
			}
		}
	}

}

