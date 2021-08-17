
project(eXl_Editor)

SOURCE_GROUP("Editor" REGULAR_EXPRESSION ".*(h|hpp|inl|cpp)")
SOURCE_GROUP("Widgets" REGULAR_EXPRESSION "eXl_Editor/.*(h|hpp|inl|cpp)")
SOURCE_GROUP("MapEditor" REGULAR_EXPRESSION "mapeditor/.*(hpp|inl|cpp)")
SOURCE_GROUP("Tileset" REGULAR_EXPRESSION "tileset/.*(hpp|inl|cpp)")
SOURCE_GROUP("MocFiles" REGULAR_EXPRESSION "moc_.*cpp")

find_package(Qt5 COMPONENTS Core Gui Widgets Opengl REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${QT5Core_INCLUDE_DIRS} ${QT5Widgets_INCLUDE_DIRS} ${QT5Gui_INCLUDE_DIRS} ${QT5OpenGL_INCLUDE_DIRS})

set(QT_LIBRARIES ${Qt5Core_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Gui_LIBRARIES} ${Qt5OpenGL_LIBRARIES})

SET(QTAppl_SRCS

eXl_Editor/array_editor.cpp
eXl_Editor/arrayitem_editor.cpp
eXl_Editor/quaternion_editor.cpp
eXl_Editor/vector_editor.cpp
eXl_Editor/enum_editor.cpp
eXl_Editor/objectptr_editor.cpp
eXl_Editor/resourcehandle_editor.cpp

mainwindow.cpp
modeltest.cpp
objectmodel.cpp
customizationmodel.cpp
objectdelegate.cpp
arraymodel.cpp
main.cpp
qtapplication.cpp
dynobjecteditor.cpp
editordef.cpp
gamewidget.cpp
gamewidgetselection.cpp
editorstate.cpp
editoricons.cpp
resourceselectionwidget.cpp
gridpainter.cpp
aspectratiowidget.cpp
archetypeeditor.cpp
projectresourcesmodel.cpp
projecteditor.cpp
scripteditor.cpp

tileset/tileselectionwidget.cpp
tileset/tileseteditor.cpp
tileset/tilinggroupeditor.cpp
tileset/tileitemdelegate.cpp
tileset/tilecollectionmodel.cpp

mapeditor/archetypecustomization.cpp
mapeditor/mapeditor.cpp
mapeditor/commonwidgets.cpp
mapeditor/tilestool.cpp
mapeditor/objectstool.cpp
mapeditor/pentoolfilter.cpp
mapeditor/terraintool.cpp
mapeditor/editortool.cpp
)

SET(HEADERS

)

SET(QTAppl_MOC_HEADERS
eXl_Editor/array_editor.h
eXl_Editor/arrayitem_editor.h
eXl_Editor/quaternion_editor.h
eXl_Editor/vector_editor.h
eXl_Editor/enum_editor.h
eXl_Editor/objectptr_editor.h
eXl_Editor/resourcehandle_editor.h

mainwindow.h
editorstate.hpp
modeltest.hpp
objectmodel.hpp
customizationmodel.hpp
objectdelegate.hpp
arraymodel.hpp
editordef.hpp
dynobjecteditor.hpp
gamewidget.hpp
gamewidgetselection.hpp
editoricons.hpp
resourceselectionwidget.hpp
gridpainter.hpp
aspectratiowidget.hpp
archetypeeditor.hpp
projectresourcesmodel.hpp
projecteditor.hpp
scripteditor.hpp

tileset/tileseteditor.hpp
tileset/tilinggroupeditor.hpp
tileset/tileitemdelegate.hpp
tileset/tilecollectionmodel.hpp
tileset/tileselectionwidget.hpp

mapeditor/archetypecustomization.hpp
mapeditor/mapeditor.hpp
mapeditor/commonwidgets.hpp
mapeditor/utils.hpp
mapeditor/tilestool.hpp
mapeditor/objectstool.hpp
mapeditor/pentoolfilter.hpp
mapeditor/terraintool.hpp
mapeditor/editortool.hpp
)

QT5_WRAP_CPP(QTAppl_MOC_SRC 
${QTAppl_MOC_HEADERS} 
OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)

add_executable(eXl_Editor ${QTAppl_MOC_HEADERS} ${QTAppl_SRCS} ${QTAppl_MOC_SRC} ${HEADERS})

SETUP_EXL_TARGET(eXl_Editor DEPENDENCIES eXl_Engine)
target_link_libraries(eXl_Editor PRIVATE ${QT_LIBRARIES})

add_custom_command(
      TARGET eXl_Editor
	  POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:eXl_Editor>/editor_rsc)

add_custom_command(
      TARGET eXl_Editor
	  POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/editor_icons.png $<TARGET_FILE_DIR:eXl_Editor>/editor_rsc)