/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#ifdef TRACE_LEAKS
#define eXl_Bullet_NEW(Type) new(MemoryManager::Allocate_Ext(sizeof(Type),__FILE__,__LINE__,FUN_STR,0,&btAlignedAllocInternal)) Type
#define eXl_Bullet_DELETE(Type,Obj) do{if(Obj)Obj->~Type();MemoryManager::Free_Ext(Obj,__FILE__,__LINE__,FUN_STR,false,&btAlignedFreeInternal);}while(false)
#else
#define eXl_Bullet_NEW(Type) new Type
#define eXl_Bullet_DELETE(Type,Obj) do{delete Obj;}while(false)
#endif


#define TO_BTQUAT(iVal) btQuaternion((iVal).w,(iVal).x,(iVal).y,(iVal).z)
#define TO_BTVECT(iVal) btVector3((iVal).x,(iVal).y,(iVal).z)

#define FROM_BTQUAT(iVal) Quaternion((iVal).w(),(iVal).x(),(iVal).y(),(iVal).z())
#define FROM_BTVECT(iVal) Vec3((iVal).x(),(iVal).y(),(iVal).z())