#pragma once

#include <engine/map/map.hpp>

#include <QWidget>
#include <QSpinBox>
#include <QComboBox>

#include "utils.hpp"

namespace eXl
{
	class TerrainWidget : public QWidget
	{
		Q_OBJECT
	public:
		TerrainWidget(QWidget* parent);

		TerrainTypeName GetCurTerrain() const { return m_CurTerrainType; }

	Q_SIGNALS:
		void onTerrainChanged();

	protected:
		QComboBox* m_TerrainSelector;
		TerrainTypeName m_CurTerrainType;
	};

	class LayerWidget : public QWidget
	{
		Q_OBJECT
	public:
		LayerWidget(QWidget* parent);

		uint8_t GetCurLayer() const { return m_CurLayer; }
		void SetLayer(uint8_t iLayer);

	Q_SIGNALS:
		void onLayerChanged(int32_t);

	protected:
		QSpinBox* m_LayerSelector = nullptr;
		uint8_t m_CurLayer = 0;
	};
}