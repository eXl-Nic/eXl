# Build for LuaBind
# Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com/
# Iowa State University HCI Graduate Program/VRAC

set(LUABIND_INCLUDE_DIR ${EXL_ROOT}/include/core/lua/)

set(LUABIND_SRCS
	lua/luabind/class.cpp
	lua/luabind/class_info.cpp
	lua/luabind/class_registry.cpp
	lua/luabind/class_rep.cpp
	lua/luabind/create_class.cpp
	lua/luabind/error.cpp
	lua/luabind/exception_handler.cpp
	lua/luabind/function.cpp
	lua/luabind/function_introspection.cpp
	lua/luabind/inheritance.cpp
	lua/luabind/link_compatibility.cpp
	lua/luabind/object_rep.cpp
	lua/luabind/open.cpp
	lua/luabind/operator.cpp
	lua/luabind/pcall.cpp
	lua/luabind/scope.cpp
	lua/luabind/set_package_preload.cpp
	lua/luabind/stack_content_by_name.cpp
	lua/luabind/weak_ref.cpp
	lua/luabind/wrapper_base.cpp)

set(LUABIND_API
	${LUABIND_INCLUDE_DIR}/luabind/back_reference_fwd.hpp
	${LUABIND_INCLUDE_DIR}/luabind/back_reference.hpp
	${LUABIND_INCLUDE_DIR}/luabind/class.hpp
	${LUABIND_INCLUDE_DIR}/luabind/class_info.hpp
	${LUABIND_INCLUDE_DIR}/luabind/config.hpp
	${LUABIND_INCLUDE_DIR}/luabind/error.hpp
	${LUABIND_INCLUDE_DIR}/luabind/error_callback_fun.hpp
	${LUABIND_INCLUDE_DIR}/luabind/exception_handler.hpp
	${LUABIND_INCLUDE_DIR}/luabind/from_stack.hpp
	${LUABIND_INCLUDE_DIR}/luabind/function.hpp
	${LUABIND_INCLUDE_DIR}/luabind/function_introspection.hpp
	${LUABIND_INCLUDE_DIR}/luabind/get_main_thread.hpp
	${LUABIND_INCLUDE_DIR}/luabind/pointer_traits.hpp
	${LUABIND_INCLUDE_DIR}/luabind/handle.hpp
	${LUABIND_INCLUDE_DIR}/luabind/luabind.hpp
	${LUABIND_INCLUDE_DIR}/luabind/lua_argument_proxy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/lua_include.hpp
	${LUABIND_INCLUDE_DIR}/luabind/lua_index_proxy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/lua_iterator_proxy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/lua_proxy_interface.hpp
	${LUABIND_INCLUDE_DIR}/luabind/lua_state_fwd.hpp
	${LUABIND_INCLUDE_DIR}/luabind/make_function.hpp
	${LUABIND_INCLUDE_DIR}/luabind/nil.hpp
	${LUABIND_INCLUDE_DIR}/luabind/object.hpp
	${LUABIND_INCLUDE_DIR}/luabind/open.hpp
	${LUABIND_INCLUDE_DIR}/luabind/operator.hpp
	${LUABIND_INCLUDE_DIR}/luabind/prefix.hpp
	${LUABIND_INCLUDE_DIR}/luabind/scope.hpp
	${LUABIND_INCLUDE_DIR}/luabind/set_package_preload.hpp
	${LUABIND_INCLUDE_DIR}/luabind/shared_ptr_converter.hpp
	${LUABIND_INCLUDE_DIR}/luabind/tag_function.hpp
	${LUABIND_INCLUDE_DIR}/luabind/typeid.hpp
	${LUABIND_INCLUDE_DIR}/luabind/lua_proxy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/version.hpp
	${LUABIND_INCLUDE_DIR}/luabind/weak_ref.hpp
	${LUABIND_INCLUDE_DIR}/luabind/wrapper_base.hpp
)
source_group(luabind/API FILES ${LUABIND_API})

set(LUABIND_DETAIL_API
	${LUABIND_INCLUDE_DIR}/luabind/detail/meta.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/call_traits.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/call_shared.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/crtp_iterator.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/call_function.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/call.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/call_member.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/class_registry.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/class_rep.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/constructor.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/conversion_storage.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/push_to_lua.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/debug.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/decorate_type.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/deduce_signature.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/enum_maker.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/format_signature.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/garbage_collector.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/inheritance.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/instance_holder.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/link_compatibility.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/make_instance.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/object.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/object_rep.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/open.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/operator_id.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/other.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/pcall.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/policy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/primitives.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/property.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/ref.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/signature_match.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/stack_utils.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/type_traits.hpp
)
source_group(luabind/Internal FILES ${LUABIND_DETAIL_API})

set(LUABIND_DEFAULT_POLICIES
	${LUABIND_INCLUDE_DIR}/luabind/detail/conversion_policies/pointer_converter.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/conversion_policies/reference_converter.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/conversion_policies/value_converter.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/conversion_policies/enum_converter.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/conversion_policies/function_converter.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/conversion_policies/conversion_base.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/conversion_policies/conversion_policies.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/conversion_policies/lua_proxy_converter.hpp
	${LUABIND_INCLUDE_DIR}/luabind/detail/conversion_policies/native_converter.hpp
	)
source_group("luabind/Default Policies" FILES ${LUABIND_DEFAULT_POLICIES} )

set(LUABIND_USER_POLICIES
	${LUABIND_INCLUDE_DIR}/luabind/yield_policy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/no_dependency.hpp
	${LUABIND_INCLUDE_DIR}/luabind/iterator_policy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/container_policy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/copy_policy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/dependency_policy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/discard_result_policy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/adopt_policy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/out_value_policy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/return_reference_to_policy.hpp
	${LUABIND_INCLUDE_DIR}/luabind/raw_policy.hpp
)
source_group("luabind/User Policies" FILES ${LUABIND_USER_POLICIES} )

set(LUABIND_EXTRA_LIBS)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251")
elseif(CMAKE_COMPILER_IS_GNUCXX)
	list(APPEND LUABIND_EXTRA_LIBS m)
endif()

SET(LUABIND_ALL_SRCS ${LUABIND_SRCS} ${LUABIND_API} ${LUABIND_DETAIL_API} ${LUABIND_DEFAULT_POLICIES} ${LUABIND_USER_POLICIES})

#add_library(luabind STATIC ${LUABIND_SRCS} ${LUABIND_API} ${LUABIND_DETAIL_API} ${LUABIND_DEFAULT_POLICIES} ${LUABIND_USER_POLICIES})
#target_include_directories(luabind PRIVATE ${INCLUDE_DIR} ${LUA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/include ${Boost_INCLUDE_DIR})
#target_link_libraries(luabind ${LUA_LIBRARIES} ${LUABIND_EXTRA_LIBS})
#
##set_property(TARGET luabind PROPERTY COMPILE_FLAGS ${CMAKE_SHARED_LIBRARY_C_FLAGS})
#set_property(TARGET luabind PROPERTY PROJECT_LABEL "LuaBind Library")
#
#set(ALLHEADERS ${LUABIND_API} ${LUABIND_DETAIL_API} PARENT_SCOPE)
#set(APIHEADERS ${LUABIND_API} PARENT_SCOPE)
#
#if(LUABIND_INSTALL)
#	if(NOT INCLUDE_DIR)
#		set(INCLUDE_DIR include)
#	endif()
#	if(NOT LIB_DIR)
#		set(LIB_DIR lib)
#	endif()
#	if(NOT ARCH_DIR)
#		set(ARCH_DIR lib)
#	endif()
#	if(NOT BIN_DIR)
#		set(BIN_DIR bin)
#	endif()
#	install(FILES ${LUABIND_API}
#		DESTINATION ${INCLUDE_DIR}/luabind
#		COMPONENT sdk)
#	install(FILES ${LUABIND_USER_POLICIES}
#		DESTINATION ${INCLUDE_DIR}/luabind
#		COMPONENT sdk)	
#	install(FILES ${LUABIND_DETAIL_API}
#		DESTINATION ${INCLUDE_DIR}/luabind/detail
#		COMPONENT sdk)
#	install(FILES ${LUABIND_DEFAULT_POLICIES}
#		DESTINATION ${INCLUDE_DIR}/luabind/detail/conversion_policies
#		COMPONENT sdk)
#	install(TARGETS luabind
#		EXPORT luabind
#		RUNTIME DESTINATION ${BIN_DIR} COMPONENT runtime
#		LIBRARY DESTINATION ${LIB_DIR} COMPONENT runtime
#		ARCHIVE DESTINATION ${ARCH_DIR} COMPONENT sdk)
#endif()

