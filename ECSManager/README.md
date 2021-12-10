# ECSManager  
---  
|    Dependencies    |          Description         |  
| ------------------ | ---------------------------- |  
|  [`EnTT`](https://github.com/skypjack/entt) | Entity Component System |  
   
##### Features  
- [x] Entity  
- [x] Components  
- [x] Events and signals  
  
##### API  
---  
> Function: `CreateEntity(id)`  
> Arg: `id` -> Identifier (integer type)  
> Description: Creates an entity using a sequence of identifiers  
> Return: A valid `ECSManager::Entity`  
> Note: When entities are Removed, their identifiers can be recycled and reused.  
```C++
Entity CreateEntity()
``` 
---  
> Function: `CreateEntity(id)`  
> Arg: `id` -> Identifier (integer type)  
> Description: Create an entity with your own identifier.  
> Return: A valid `ECSManager::Entity`  
```C++
template <typename Type, typename = std::enable_if_t<std::is_integral_v<Type>>>
Entity CreateEntity(const Type id);
```
  
---  
## TODO..  
---  