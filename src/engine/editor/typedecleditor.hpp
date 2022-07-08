#pragma once

#include <QWidget>

#include <engine/common/project.hpp>

class QTableWidget;

namespace eXl
{
	class TypeDeclEditor : public QWidget
	{
		Q_OBJECT
	public:

    static TypeDeclEditor* Create(QWidget* iParent);
    
    void SetDecl(Project::TypeDecl const& iDecl);

    Project::TypeDecl const& GetDecl() const { return m_Decl; }

    void Clear();

  Q_SIGNALS:
    void OnDeclChange();

	protected:
    TypeDeclEditor(QWidget* iParent);

    void Build(QLayout* iLayout);

    void UpdateEditedProperty();

    QTableWidget* m_PropDataView;
    QMetaObject::Connection m_PropDataChangeCb;
    Project::TypeDecl m_Decl;

    QStringList m_TypeNames;
    QStringList m_TypeDisplayNames;
	};
}