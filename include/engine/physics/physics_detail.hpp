#pragma once

#ifdef TRACE_LEAKS
#define eXl_Bullet_NEW(Type) new(MemoryManager::Allocate_Ext(sizeof(Type),__FILE__,__LINE__,FUN_STR,0,&btAlignedAllocInternal)) Type
#define eXl_Bullet_DELETE(Type,Obj) do{if(Obj)Obj->~Type();MemoryManager::Free_Ext(Obj,__FILE__,__LINE__,FUN_STR,false,&btAlignedFreeInternal);}while(false)
#else
#define eXl_Bullet_NEW(Type) new Type
#define eXl_Bullet_DELETE(Type,Obj) do{delete Obj;}while(false)
#endif


#define TO_BTQUAT(x) btQuaternion((x).X(),(x).Y(),(x).Z(),(x).W())
#define TO_BTVECT(x) btVector3((x).X(),(x).Y(),(x).Z())

#define FROM_BTQUAT(X) Quaternionf((X).w(),(X).x(),(X).y(),(X).z())
#define FROM_BTVECT(X) Vector3f((X).x(),(X).y(),(X).z())