#include <Windows.h>

#include "../../API/RainmeterAPI.h"

// Overview: This is a blank canvas on which to build your plugin.

// Note: GetString and ExecuteBang have been commented out. If you need
// GetString and/or ExecuteBang and you have read what they are used for
// from the SDK docs, uncomment the function(s). Otherwise leave them
// commented out (or get rid of them)!

class measure_data{

private:


public:


};

// called when skin is launched/refreshed
// read local skin file to create Rainmeter runtime measure object, which holds
// data of the measure section, including its name, all the "option=value"
// pairs specified in the local skin file, and so on.
// use the RmReadString/RmReadInt-like functions to read value of a option from
// this object.
PLUGIN_EXPORT void Initialize(void **data, void *rm){
	// you can allocate a data block to store any data of the measure(plugin)
	// like this, and the data pointer is kept by Rainmeter.
	*data = reinterpret_cast<void *>(new measure_data);
}

// called after Initialize.
// while dynamic variables is enabled, the Rainmeter runtime measure object can
// be changed in runtime, and this function will be called on every update cycle
// before Reload to synchronize some data.
PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue){
	
}

// call on every update cycle
PLUGIN_EXPORT double Update(void* data){
	
	return 0.0;
}

// return a string representation of the measure
PLUGIN_EXPORT LPCWSTR GetString(void* data){

	return L"";
}

// 
PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args){
	
}

// 
PLUGIN_EXPORT void Finalize(void* data){
	// release data block
	delete reinterpret_cast<measure_data *>(data);
}
