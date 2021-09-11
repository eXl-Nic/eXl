
#include "commonwidgets.hpp"

#include <QHBoxLayout>
#include <QLabel>

namespace eXl
{
	LayerWidget::LayerWidget(QWidget* parent)
		: QWidget(parent)
	{
		QHBoxLayout* layout = new QHBoxLayout(this);
		m_LayerSelector = new QSpinBox;
		m_LayerSelector->setMinimum(0);
		m_LayerSelector->setMaximum(16);

		layout->addWidget(new QLabel("Layer : ", this));
		layout->addWidget(m_LayerSelector);

		QObject::connect(m_LayerSelector, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](int value)
		{
			m_CurLayer = value;
			emit onLayerChanged(value);
		});
	}

	void LayerWidget::SetLayer(uint8_t iLayer)
	{
		m_CurLayer = iLayer;
		m_LayerSelector->setValue(iLayer);
	}

	TerrainWidget::TerrainWidget(QWidget* parent)
		: QWidget(parent)
	{
		QHBoxLayout* layout = new QHBoxLayout(this);

    Vector<TerrainType> types = TerrainType::GetTerrainTypes();

		m_TerrainSelector = new QComboBox;
		layout->addWidget(new QLabel("Terrain : ", this));
		layout->addWidget(m_TerrainSelector);
		
    m_CurTerrainType = types[0].m_TerrainType;
    for (auto const& type : types)
    {
      m_TerrainSelector->addItem(type.m_TerrainType.get().c_str());
    }

		QObject::connect(m_TerrainSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, types](int iIndex)
		{
      m_CurTerrainType = types[iIndex].m_TerrainType;
			emit onTerrainChanged();
		});
	}
}