# HelenaSystems  
This repository contains set of `Systems` for solve a specific task.  
The provided `Systems` are not standalone and are designed for use with the [`HelenaFramework`](https://github.com/NIKEA-SOFT/HelenaFramework).  
  
---  
|    Systems    |            Description         |  
| ------------- | ------------------------------ |  
|  [`PluginManager`](https://github.com/NIKEA-SOFT/HelenaSystems/PluginManager) | Runtime (dll/so) plugins manager without abstraction |  
|  [`ResourceManager`](https://github.com/NIKEA-SOFT/HelenaSystems/ResourceManager) | Storage for resources from data |  
|  [`ECSManager`](https://github.com/NIKEA-SOFT/HelenaSystems/ECSManager) | System wrapper of [`EnTT`](https://github.com/skypjack/entt) |  
  
**Usage:**  
1\) Select a `System` folder from the list and copy to your project.  
2\) Include header in your code.  
3\) Use `HelenaFramework` for register or get system.  
  
**Example:**  
```C++ 
#include <Helena/Engine/Engine.hpp>
#include <MyProject/Systems/ResourceManager/ResourceManager.hpp>

int main(int argc, char** argv)
{
    Helena::Engine::Context::Initialize();			// Initialize Context (Context used in Engine)
    Helena::Engine::Context::SetMain([]()			// Register systems happen in this callback
    {
        // Register all used systems
        Helena::Engine::RegisterSystem<Helena::Systems::EntityComponent>();
        
        // Now we can get the system from anywhere
        // Helena::Engine::GetSystem<Helena::Systems::ResourceManager>();
    });

    // Engine loop
    while(Helena::Engine::Heartbeat()) {}
    return 0;
}
```  
  
---  
> **Note:** You can add your implementations that solve a specific task to this repository. 
> **License:** The `Systems` developed by `NIKEA-SOFT` are licensed by `MIT`.  
> Libraries not owned by the developer `NIKEA-SOFT` contain their own license agreement.  
> By using `Systems` that are implemented using `Dependencies` libraries,  
> you agree to the agreements of the authors of the `Dependencies` libraries and our agreement.  