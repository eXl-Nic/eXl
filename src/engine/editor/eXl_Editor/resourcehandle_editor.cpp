#include "resourcehandle_editor.h"

#include "../editorstate.hpp"

#include <core/type/resourcehandletype.hpp>
#include <core/resource/resourcemanager.hpp>

#include <QHBoxLayout>

#include <QComboBox>

ResourceHandle_Editor::ResourceHandle_Editor(QWidget *parent)
  :DynObjectEditor(parent)
{
  QHBoxLayout* hLayout = new QHBoxLayout;
  hLayout->setContentsMargins(0,0,0,0);
  hLayout->setAlignment(Qt::AlignLeft);
  setLayout(hLayout);
}

void ResourceHandle_Editor::OnEditingFinished()
{
  eXl::ResourceHandleType const* typeId = eXl::ResourceHandleType::DynamicCast(m_Obj.GetType());
  auto projectResources = eXl::EditorState::GetState()->GetProjectResourcesModel();
  auto* resourceModel = static_cast<eXl::ProjectResourcesModel::TypeFilter*>(m_ResourceCombo->model());

  eXl::Resource::UUID const* resourceId = resourceModel->GetResourceIDFromIndex(resourceModel->index(m_ResourceCombo->currentIndex(), 0, QModelIndex()));

  if (resourceId != nullptr)
  {
    typeId->SetUUID(m_Obj.GetBuffer(), *resourceId);
  }
  else
  {
    typeId->SetUUID(m_Obj.GetBuffer(), eXl::Resource::UUID());
  }

  emit editingFinished();
}

void ResourceHandle_Editor::UpdateView()
{
  unsigned int numComp = 0;
  eXl::ResourceHandleType const* typeId = eXl::ResourceHandleType::DynamicCast(m_Obj.GetType());
  m_ResourceCombo = new QComboBox(this);
  auto projectResources = eXl::EditorState::GetState()->GetProjectResourcesModel();
  auto* resourceModel = projectResources->MakeFilteredModel(m_ResourceCombo, eXl::ResourceManager::GetLoaderFromRtti(typeId->GetMinimalRtti()), true);
  m_ResourceCombo->setModel(resourceModel);
  
  eXl::Resource::UUID const& uuid = typeId->GetUUID(m_Obj.GetBuffer());
  if (uuid.IsValid())
  {
    m_ResourceCombo->setCurrentIndex(resourceModel->GetIndexFromUUID(uuid).row());
  }
  QObject::connect(m_ResourceCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int)
  {
    OnEditingFinished();
  });

  layout()->addWidget(m_ResourceCombo);
}
