/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

struct DspInstance::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(DspInstance, processBlock);
	API_VOID_METHOD_WRAPPER_2(DspInstance, prepareToPlay);
	API_VOID_METHOD_WRAPPER_2(DspInstance, setParameter);
	API_METHOD_WRAPPER_1(DspInstance, getParameter);
	API_METHOD_WRAPPER_0(DspInstance, getInfo);
};


DspInstance::DspInstance(const DspFactory *f, const String &moduleName_) :
ConstScriptingObject(nullptr, NUM_API_FUNCTION_SLOTS),
moduleName(moduleName_),
factory(const_cast<DspFactory*>(f))
{
	if (factory != nullptr)
	{
		object = factory->createDspBaseObject(moduleName);

		if (object != nullptr)
		{
			ADD_API_METHOD_1(processBlock);
			ADD_API_METHOD_2(prepareToPlay);
			ADD_API_METHOD_2(setParameter);
			ADD_API_METHOD_1(getParameter);
			ADD_API_METHOD_0(getInfo);

			for (int i = 0; i < object->getNumConstants(); i++)
			{
				char nameBuffer[64];
				int nameLength = 0;

				object->getIdForConstant(i, nameBuffer, nameLength);
				String name(nameBuffer, (size_t)nameLength);

				int intValue;
				if (object->getConstant(i, intValue))
				{
					addConstant(name, var(intValue));
					continue;
				}
				
				float floatValue;
				if (object->getConstant(i, floatValue))
				{
					addConstant(name, var(floatValue));
					continue;
				}
				
				char stringBuffer[512];
				size_t stringBufferLength;

				if (object->getConstant(i, stringBuffer, stringBufferLength))
				{
					String text(stringBuffer, stringBufferLength);
					addConstant(name, var(text));
					continue;
				}

				
				float *externalData;
				int externalDataSize;
				
				if (object->getConstant(i, &externalData, externalDataSize))
				{
					VariantBuffer::Ptr b = new VariantBuffer(externalData, externalDataSize);
					addConstant(name, var(b));
					continue;
				}
			}
		}
	};
}


void DspInstance::processBlock(const var &data)
{
	if (object != nullptr)
	{
		if (data.isArray())
		{
			Array<var> *a = data.getArray();
			float *sampleData[4]; // this is an arbitrary amount, but it should be OK...
			int numSamples = -1;

			if (a == nullptr)
				throwError("processBlock must be called on array of buffers");

			for (int i = 0; i < jmin<int>(4, a->size()); i++)
			{
				VariantBuffer *b = a->getUnchecked(i).getBuffer();

				if (b != nullptr)
				{
					if (numSamples != -1 && b->size != numSamples)
						throwError("Buffer size mismatch");

					numSamples = b->size;
				}
				else throwError("processBlock must be called on array of buffers");

				sampleData[i] = b->buffer.getWritePointer(0);
			}

			object->processBlock(sampleData, a->size(), numSamples);
		}
		else if (data.isBuffer())
		{
			VariantBuffer *b = data.getBuffer();

			if (b != nullptr)
			{
				float *sampleData[1] = { b->buffer.getWritePointer(0) };
				const int numSamples = b->size;

				object->processBlock(sampleData, 1, numSamples);
			}
		}
		else throwError("Data Buffer is not valid");
	}
}

void DspInstance::setParameter(int index, var newValue)
{
	if (object != nullptr)
	{
		object->setParameter(index, newValue);
	}
}

var DspInstance::getParameter(int index) const
{
	if (object != nullptr)
	{
		return object->getParameter(index);
	}

	return var::undefined();
}

void DspInstance::assign(const int index, var newValue)
{
	setParameter(index, newValue);
}

var DspInstance::getAssignedValue(int index) const
{
	return getParameter(index);
}

int DspInstance::getCachedIndex(const var &name) const
{
	if (object != nullptr)
	{
		for (int i = 0; i < object->getNumParameters(); i++)
		{
			if (name.toString() == object->getIdForParameter(i).toString())
			{
				return i;
			}
		}
	}

	return -1;
}

void DspInstance::operator<<(var &/*data*/) const
{
	// DSP_TODO
}

var DspInstance::getInfo() const
{
	if (object != nullptr)
	{
		String info;

		info << "Name: " + moduleName << "\n";

		info << "Parameters: " << String(object->getNumParameters()) << "\n";

		for (int i = 0; i < object->getNumParameters(); i++)
		{
			const String line = String("Parameter #" + String(i) + ": ") + object->getIdForParameter(i).toString() + String(", current value: ") + String(object->getParameter(i)) + String("\n");

			info << line;
		}

		info << "\n";

		info << "Constants: " << String(object->getNumConstants()) << "\n";

		for (int i = 0; i < object->getNumConstants(); i++)
		{
			info << "Constant #" << String(i) << ": " << getConstantName(i).toString() << +" = " << getConstantValue(i).toString() << "\n";
		}

		return var(info);
	}

	return var("No module loaded");
}

void DspInstance::operator>>(const var &data)
{
	processBlock(data);
}

DspInstance::~DspInstance()
{
	for (int i = 0; i < object->getNumConstants(); i++)
	{
		if (getConstantValue(i).isBuffer())
			getConstantValue(i).getBuffer()->referToData(nullptr, 0);
	}

	if (factory != nullptr)
	{
		factory->destroyDspBaseObject(object);
		factory = nullptr;
	}
}

void DspInstance::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	if (object != nullptr)
	{
		object->prepareToPlay(sampleRate, samplesPerBlock);

		for (int i = 0; i < object->getNumConstants(); i++)
		{
			if (getConstantValue(i).isBuffer())
			{
				float* data;
				int size;

				object->getConstant(i, &data, size);

				getConstantValue(i).getBuffer()->referToData(data, size);
			}
		}
	}
}

typedef DspBaseObject*(*createDspModule_)(const char *);


class DspFactory::Handler
{
public:

	Handler();
	~Handler();

	DspInstance *createDspInstance(const String &factoryName, const String& factoryPassword, const String &moduleName);

	template <class T> static void registerStaticFactory(Handler *instance);
    
	/** Returns a factory with the given name.
	*
	*	It looks for static factories first. If no static library is found, it searches for opened dynamic factories.
	*	If no dynamic factory is found, it will open the dynamic library at the standard path and returns this instance.
	*/
	DspFactory *getFactory(const String &name, const String& password);

    void getAllStaticLibraries(StringArray &libraries)
    {
        for(int i = 0; i < staticFactories.size(); i++)
        {
            libraries.add(staticFactories[i]->getId().toString());
        }
    };
    
    void getAllDynamicLibraries(StringArray &libraries)
    {
        for(int i = 0; i < loadedPlugins.size(); i++)
        {
            libraries.add(loadedPlugins[i]->getId().toString());
        }
    }
    
                          
private:

	static void registerStaticFactories(Handler *instance);

    
    
	ReferenceCountedArray<DspFactory> staticFactories;
	ReferenceCountedArray<DspFactory> loadedPlugins;
};

class DspFactory::LibraryLoader : public DynamicObject
{
public:

	LibraryLoader()
	{
		ADD_DYNAMIC_METHOD(load);
        ADD_DYNAMIC_METHOD(list);
	}

	var load(const String &name, const String &password)
	{
		DspFactory *f = dynamic_cast<DspFactory*>(handler->getFactory(name, password));
		return var(f);
	}
    
    var list()
    {
        StringArray s1, s2;
        
        handler->getAllStaticLibraries(s1);
        handler->getAllDynamicLibraries(s2);
        
        String output = "Available static libraries: \n";
        output << s1.joinIntoString("\n");
        
        output << "\nAvailable dynamic libraries: " << "\n";
        output << s2.joinIntoString("\n");
        
        return var(output);
    }

private:

	struct Wrapper
	{
		DYNAMIC_METHOD_WRAPPER_WITH_RETURN(LibraryLoader, load, ARG(0).toString(), ARG(1).toString());
        DYNAMIC_METHOD_WRAPPER_WITH_RETURN(LibraryLoader, list);
	};

	SharedResourcePointer<DspFactory::Handler> handler;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryLoader)
};

struct DspFactory::Wrapper
{
	DYNAMIC_METHOD_WRAPPER_WITH_RETURN(DspFactory, createModule, ARG(0).toString());
	DYNAMIC_METHOD_WRAPPER_WITH_RETURN(DspFactory, getModuleList);
};

DspFactory::DspFactory() :
DynamicObject()
{
	ADD_DYNAMIC_METHOD(createModule);
	ADD_DYNAMIC_METHOD(getModuleList);
}

DynamicDspFactory::DynamicDspFactory(const String &name_, const String& password) :
name(name_)
{
#if JUCE_WINDOWS

	//const File path = File("D:\\Development\\HISE modules\\extras\\script_module_template\\Builds\\VisualStudio2013\\Debug");

	const File path = File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Hart Instruments/dll/");

#if JUCE_32BIT

	const String libraryName = name + "_x86.dll";

#else

	const String libraryName = name + "_x64.dll";

#endif
	




#else

    const File path = File::getSpecialLocation(File::SpecialLocationType::commonApplicationDataDirectory).getChildFile("Application Support/Hart Instruments/lib");
    
    const String libraryName = name + ".dylib";
    
#endif

    const String fullLibraryPath = path.getChildFile(libraryName).getFullPathName();
    
	File f(fullLibraryPath);

	if (!f.existsAsFile()) throw String("Library " + name + " was not found");

	library = new DynamicLibrary();
	library->open(fullLibraryPath);

	using pFunc = bool(*)(const char*);

	pFunc func = (pFunc)library->getFunction("matchPassword");

	if (func != nullptr)
	{
		if (password.isEmpty())
			throw String("This Library is locked. You need a password to open it");

		const bool passwordMatches = func(password.getCharPointer());

		if (!passwordMatches)
			throw String("Wrong password for locked DLL. Abort loading");
	}
	

	initialise();

	ADD_DYNAMIC_METHOD(createModule);
}

void GlobalScriptCompileBroadcaster::createDummyLoader()
{
    dummyLibraryLoader = new DspFactory::LibraryLoader();
}

DspBaseObject * DynamicDspFactory::createDspBaseObject(const String &moduleName) const
{
	if (library != nullptr)
	{
		createDspModule_ c = (createDspModule_)library->getFunction("createDspObject");

		if (c != nullptr)
		{
			return c(moduleName.getCharPointer());
		}
	}

	return nullptr;
}

typedef void(*destroyDspModule_)(DspBaseObject*);

void DynamicDspFactory::destroyDspBaseObject(DspBaseObject *object) const
{
	if (library != nullptr)
	{
		destroyDspModule_ d = (destroyDspModule_)library->getFunction("destroyDspObject");

		if (d != nullptr && object != nullptr)
		{
			d(object);
		}
	}
}

typedef void(*init_)();

void DynamicDspFactory::initialise()
{
	if (library != nullptr)
	{
		init_ d = (init_)library->getFunction("initialise");

		if (d != nullptr)
		{
			d();
		}
		else
		{
			throw String("initialise() not implemented in Dynamic Library " + name);
		}
	}

}

var DynamicDspFactory::createModule(const String &moduleName) const
{
	DspInstance *i = new DspInstance(this, moduleName);

	return var(i);
}

typedef const Array<Identifier> &(*getModuleList_)();

var DynamicDspFactory::getModuleList() const
{
	if (library != nullptr)
	{
		getModuleList_ d = (getModuleList_)library->getFunction("getModuleList");

		if (d != nullptr)
		{
			const Array<Identifier> *ids = &d();

			Array<var> a;

			for (int i = 0; i < ids->size(); i++)
			{
				a.add(ids->getUnchecked(i).toString());
			}

			return var(a);
		}
		else
		{
			throw String("getModuleList not implemented in Dynamic Library " + name);
		}
	}

	return var::undefined();
}

DspBaseObject * StaticDspFactory::createDspBaseObject(const String &moduleName) const
{
	return factory.createFromId(moduleName);
}

void StaticDspFactory::destroyDspBaseObject(DspBaseObject* handle) const
{
	delete handle;
}

var StaticDspFactory::getModuleList() const
{
	Array<var> moduleList;

	for (int i = 0; i < factory.getIdList().size(); i++)
	{
		moduleList.add(factory.getIdList().getUnchecked(i).toString());
	}

	return var(moduleList);
}

var StaticDspFactory::createModule(const String &name) const
{
	DspInstance *instance = new DspInstance(this, name);

	return var(instance);
}


DspFactory::Handler::Handler()
{
	registerStaticFactories(this);
}

DspFactory::Handler::~Handler()
{
	loadedPlugins.clear();
}

DspInstance * DspFactory::Handler::createDspInstance(const String &factoryName, const String& factoryPassword, const String &moduleName)
{
	return new DspInstance(getFactory(factoryName, factoryPassword), moduleName);
}


template <class T>
void DspFactory::Handler::registerStaticFactory(Handler *instance)
{
	StaticDspFactory* staticFactory = new T();

	staticFactory->registerModules();

	instance->staticFactories.add(staticFactory);
}

DspFactory * DspFactory::Handler::getFactory(const String &name, const String& password)
{
	Identifier id(name);

	for (int i = 0; i < staticFactories.size(); i++)
	{
		if (staticFactories[i]->getId() == id)
		{
			return staticFactories[i];
		}
	}

	for (int i = 0; i < loadedPlugins.size(); i++)
	{
		if (loadedPlugins[i]->getId() == id)
		{
			return loadedPlugins[i];
		}
	}

	try
	{
		ScopedPointer<DynamicDspFactory> newLib = new DynamicDspFactory(name, password);
		loadedPlugins.add(newLib.release());
		return newLib;
	}
	catch (String errorMessage)
	{
		throw errorMessage;
		return nullptr;
	}
	
	
}


void DspFactory::Handler::registerStaticFactories(Handler *instance)
{
	registerStaticFactory<HiseCoreDspFactory>(instance);
}