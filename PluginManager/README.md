# PluginManager
This system provides the ability to load dynamic libraries as of plugins.  
Due to the architecture of the [HelenaFramework](https://github.com/NIKEA-SOFT/HelenaFramework), you can get rid of the need for three-level abstraction and linking.  

##### Features
- [x] Support .dll/.so
- [x] Runtime enable/disable
- [x] Without abstraction, using systems inside plugins  

| Events | README |
| :------: | :------: |
| `Helena::Events::PluginManager::Load` | Triggered when the plugin is loaded successfully (see API: `Load`) |  

##### API
---
> Description: Loads plugin at the specified path  
> Function: `Load(path, name)`  
> Arg: `path` : Path where the plugins are located  
> Arg: `name` : Plugin name (without extension)  
> Return `bool` : true - success, false - failure  
> Note: You don't need to specify the extension, it is detected automatically  
```C++
bool Load(const std::string_view path, const PluginName& name)
```
---
> Description: Load plugin  
> Function: `Load(name)`  
> Arg: `name` : Plugin name (without extension)  
> Return `bool` : true - success, false - failure  
> Note:  
> You don't need to specify the extension, it is detected automatically  
> The path relative to the working directory is used  
```C++
bool Load(const PluginName& name)
```
---
> Description: Initialize plugin  
> Function: `Init(name)`  
> Arg: `name` : Plugin name (without extension)  
> Return `bool`: true - success, false - failure  
> Note: The init of the plugin calls the entry point inside the module, the function: `PluginInit`  
```C++
bool Init(const PluginName& name)
```
---
> Description: Finalize plugin  
> Function: `End(name)`  
> Arg: `name` : Plugin name (without extension)  
> Return `bool`: true - success, false - failure  
> Note: The finalize of the plugin calls the exit point inside the module, the function: `PluginEnd`  
```C++
bool End(const PluginName& name)
```
---
> Description: Check if the plugin is loaded  
> Function: `Has(name)`  
> Arg: `name` : Plugin name (without extension)  
> Return `bool`: true - success, false - failure  
> Note: Check if the plugin exists before calling `Init`  
```C++
bool Has(const PluginName& name)
```
---
> Description: Check if the plugin is initialized  
> Function: `Initialized(name)`  
> Arg: `name` : Plugin name (without extension)  
> Return `bool`: true - success, false - failure  
> Note:  
> Check if the plugin is initialized before calling `End`  
> This method already includes the `Has` check  
```C++
bool Initialized(const PluginName& name)
```
---  
