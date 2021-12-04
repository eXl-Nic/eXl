#include "tileseteditor.hpp"
#include "tilecollectionmodel.hpp"
#include "tileselectionwidget.hpp"

#include <editor/editordef.hpp>
#include <editor/aspectratiowidget.hpp>
#include <editor/gamewidget.hpp>
#include <editor/gamewidgetselection.hpp>
#include <editor/gridpainter.hpp>
#include <editor/objectdelegate.hpp>
#include <editor/objectmodel.hpp>
#include <editor/eXl_Editor/ui_tileset_toolbox.h>

#include <engine/common/transforms.hpp>
#include <engine/common/transforms.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/tileset.hpp>
#include <math/mathtools.hpp>

#include <QTabWidget>
#include <QTreeView>
#include <QTableView>
#include <QListView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QToolbar>
#include <QLabel>
#include <QFileDialog>
#include <QScrollArea>

namespace eXl
{
  class TilesetToolboxWidget : public QWidget
  {
  public:

    TilesetToolboxWidget(QWidget* iParent)
      : QWidget(iParent)
    {
      m_Ui.setupUi(this);
    }

    Ui::TilesetToolbox m_Ui;
  };

  class TilesetEditorHandler : public ResourceEditorHandler
  {
  public:

     TilesetEditorHandler()
       : ResourceEditorHandler(Tileset::StaticLoaderName())
     {}

    virtual DocumentState* CreateNewDocument()
    {
      QString resourceLoaderName = m_Loader.get().c_str();
      QString title("New ");
      title.append(QString::fromUtf8(Tileset::StaticLoaderName().get().c_str()));

      QString file = QFileDialog::getSaveFileName(nullptr, title,
        QString::fromStdString(EditorState::GetProjectDirectory().string()),
        "Resource file (*.eXlAsset)");

      //QString file = QFileDialog::getOpenFileName(nullptr, title, 
      //  QString::fromStdString(EditorState::GetProjectDirectory().string()),
      //  "Image file (*.png *.jpg *.jpeg)");
      //

      if (file.isEmpty())
      {
        return nullptr;
      }

      Path newResourcePath(file.toStdString());
      if (Filesystem::exists(newResourcePath))
      {
        LOG_ERROR << "Cannot create new resource over already exising one" << "\n";
        return nullptr;
      }

      //Path imagePath(file.toStdString());
      //if (Filesystem::exists(Tileset::ResourcePathFromImagePath(imagePath)))
      //{
      //  LOG_ERROR << "Cannot create new resource over already exising one" << "\n";
      //  return nullptr;
      //}

      if (!IsInProjectFolder(newResourcePath))
      {
        LOG_ERROR << "Path " << newResourcePath.string() << " is not contained in the project folder" << "\n";
        return nullptr;
      }

      Path newResourceDir = Filesystem::absolute(Filesystem::canonical(newResourcePath.parent_path()));
      Path resourceName = newResourcePath.filename();
      resourceName.replace_extension();
      Tileset* newTileset = Tileset::Create(newResourceDir, resourceName.string().c_str());
      if (newTileset)
      {
        DocumentState* document = new DocumentState(*newTileset);
        return document;
      }
      return nullptr;
    }

    virtual ResourceEditor* CreateEditor(QWidget* iParent, DocumentState* iDoc)
    {
      return new TilesetEditor(iParent, iDoc);
    }

  };

  ResourceEditorHandler& TilesetEditor::GetEditorHandler()
  {
    static TilesetEditorHandler s_Handler;
    return s_Handler;
  }

	struct TilesetEditor::Impl
	{
		Impl()
      : m_World(EngineCommon::GetComponents())
		{
			m_Transforms = m_World.AddSystem(std::make_unique<Transforms>());
			m_Gfx = m_World.AddSystem(std::make_unique<GfxSystem>(*m_Transforms));
      m_World.AddSystem(std::make_unique<GameDatabase>(EditorState::GetProjectProperties()));
		}
		World m_World;
		Transforms* m_Transforms;
		GfxSystem* m_Gfx;


    ObjectHandle m_ImageHandle;
    ObjectHandle m_AnimHandle;

    QModelIndex m_TileSelection;

    TileCollectionModel* m_TileCollectionModel;
    QTreeView* m_TileDataView;
    //QListView* m_TilesetCollectionView;
    TileSelectionWidget* m_TilesetCollectionView;
    MouseSelectionFilter* m_Selection;
    SelectionPainter* m_SelPainter;
    GameWidget* m_MainImageView;
    GameWidget* m_TilePreview;
    GridPainter* m_GridPainter;

    boost::optional<ImageName> m_CurrentDisplayedImage;
    //AspectRatioWidget* m_RatioWidgetImage;

    boost::optional<TileName> m_CurrentDisplayedTile;
    AspectRatioWidget* m_RatioWidgetTile;

    Tileset* m_Tileset;

    void UpdateTilePreview(QModelIndex iIndex);
    void UpdateTile(QModelIndex iIndex);
    void UpdateImage();
    void RefreshSelectionBox();
    void RefreshTilesFromSelection();
    void Bootstrap();

    Tile m_CurrentEditedTile;
    TilesetEditor* m_Editor;
    TilesetToolboxWidget* m_Toolbox;

    Vector2i m_GridSize = Vector2i::ONE;
    Vector2i m_SelOffset = Vector2i::ZERO;
    Vector<Vector2i> m_CurBoxes;
    AABB2Di m_CurrentSelection;
	};

  void TilesetEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

	TilesetEditor::TilesetEditor(QWidget* iParent, DocumentState* iTilesetDoc)
		: ResourceEditor(iParent, iTilesetDoc)
		, m_Impl(new Impl)
	{
    m_Impl->m_Editor = this;
    m_Impl->m_Tileset = Tileset::DynamicCast(iTilesetDoc->GetResource());

    

		QSplitter* rootSplitter = new QSplitter(Qt::Horizontal, this);
    QSplitter* viewSplitter = new QSplitter(Qt::Vertical, this);
    QSplitter* dataSplitter = new QSplitter(Qt::Vertical, this);

    QWidget* tilesetCollection = new QWidget(this);
    QVBoxLayout* tilesetCollectionLayout = new QVBoxLayout(this);
    tilesetCollection->setLayout(tilesetCollectionLayout);
    QToolBar* tilesetCollectionTool = new QToolBar(this);
    TileSelectionWidget::Conf widgetConf;
    widgetConf.m_ComboForTiles = false;
    widgetConf.m_SelectTileset = false;
    ResourceHandle<Tileset> handle;
    handle.Set(m_Impl->m_Tileset);
    m_Impl->m_TilesetCollectionView = new TileSelectionWidget(this, widgetConf, handle, TileName());
    //m_Impl->m_TileCollectionModel = TileCollectionModel::Create(this, m_Impl->m_Tileset);
    m_Impl->m_TileCollectionModel = m_Impl->m_TilesetCollectionView->GetTileCollection();
    //m_Impl->m_TilesetCollectionView = new QListView(this);
    //m_Impl->m_TilesetCollectionView->setModel(m_Impl->m_TileCollectionModel);
    //m_Impl->m_TilesetCollectionView->setSelectionModel(new QItemSelectionModel(m_Impl->m_TilesetCollectionView->model()));

    //QObject::connect(m_Impl->m_TilesetCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
    QObject::connect(m_Impl->m_TilesetCollectionView, &TileSelectionWidget::onTileChanged, [this]()
    {
      //if (iSelected.isEmpty())
      TileName name = m_Impl->m_TilesetCollectionView->GetTileName();
      if(name == TileName())
      {
        ObjectModel::ClearModelFromView(m_Impl->m_TileDataView);
      }
      else
      {
        //if (iSelected.indexes().size() == 1)
        {

          QModelIndex index = m_Impl->m_TileCollectionModel->GetIndexFromName(name);
          
          if(Tile const* selTile = m_Impl->m_TileCollectionModel->GetTileFromIndex(index))
          {
            m_Impl->m_GridSize = selTile->m_Size;
            m_Impl->m_CurBoxes = selTile->m_Frames;
            m_Impl->m_Toolbox->m_Ui.gridSizeX->setValue(m_Impl->m_GridSize.X());
            m_Impl->m_Toolbox->m_Ui.gridSizeY->setValue(m_Impl->m_GridSize.Y());
            if (!m_Impl->m_CurBoxes.empty())
            {
              if (m_Impl->m_GridSize.X() != 0)
              {
                m_Impl->m_SelOffset.X() = m_Impl->m_CurBoxes[0].X() % m_Impl->m_GridSize.X();
              }
              else
              {
                m_Impl->m_SelOffset.X() = 0;
              }
              if (m_Impl->m_GridSize.Y() != 0)
              {
                m_Impl->m_SelOffset.Y() = m_Impl->m_CurBoxes[0].Y() % m_Impl->m_GridSize.Y();
              }
              else
              {
                m_Impl->m_SelOffset.Y() = 0;
              }
            }
            else
            {
              m_Impl->m_SelOffset.X() = 0;
              m_Impl->m_SelOffset.Y() = 0;
            }
            m_Impl->m_Toolbox->m_Ui.offsetX->setValue(m_Impl->m_SelOffset.X());
            m_Impl->m_Toolbox->m_Ui.offsetY->setValue(m_Impl->m_SelOffset.Y());
            m_Impl->RefreshSelectionBox();
          }
          m_Impl->UpdateTile(index);
        }
      }
    });

    QObject::connect(m_Impl->m_TileCollectionModel, &QAbstractItemModel::dataChanged, [this](QModelIndex const& iIndex, QModelIndex const&)
    {
      m_Impl->UpdateTile(iIndex);
      m_Impl->m_Editor->ModifyResource();
    });

    QObject::connect(m_Impl->m_TileCollectionModel, &QAbstractItemModel::rowsInserted, [this]()
    {
      m_Impl->m_Editor->ModifyResource();
    });
    QObject::connect(m_Impl->m_TileCollectionModel, &QAbstractItemModel::rowsRemoved, [this]()
    {
      m_Impl->m_Editor->ModifyResource();
    });

    tilesetCollectionLayout->addWidget(tilesetCollectionTool);
    tilesetCollectionLayout->addWidget(m_Impl->m_TilesetCollectionView);

    tilesetCollectionTool->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Add New Entry", [this]
    {
      uint32_t curRowCount = m_Impl->m_TileCollectionModel->rowCount(QModelIndex());
      m_Impl->m_TileCollectionModel->insertRow(curRowCount);

      if (auto newName = m_Impl->m_TileCollectionModel->GetTileNameFromIndex(m_Impl->m_TileCollectionModel->index(curRowCount, 0)))
      {
        Tile newTile;
        if (m_Impl->m_CurrentDisplayedImage)
        {
          newTile.m_ImageName = *m_Impl->m_CurrentDisplayedImage;
        }
        newTile.m_Size = m_Impl->m_GridSize;
        newTile.m_Frames = m_Impl->m_CurBoxes;
        m_Impl->m_Tileset->AddTile(*newName, newTile);
      }
    });

    tilesetCollectionTool->addAction(style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Entry", [this]
    {
      //QModelIndex currentSelection = m_Impl->m_TilesetCollectionView->selectionModel()->currentIndex();
      QModelIndex currentSelection = m_Impl->m_TileCollectionModel->GetIndexFromName(m_Impl->m_TilesetCollectionView->GetTileName());
      if (currentSelection.isValid())
      {
        m_Impl->m_TileCollectionModel->removeRow(currentSelection.row(), currentSelection.parent());
      }
    });

		m_Impl->m_TileDataView = new QTreeView(this);
    m_Impl->m_TileDataView->setItemDelegate(new ObjectDelegate);
    

    dataSplitter->addWidget(tilesetCollection);
    dataSplitter->addWidget(m_Impl->m_TileDataView);

    {
      m_Impl->m_MainImageView = new GameWidget(this);
      m_Impl->m_MainImageView->SetGfxSystem(m_Impl->m_Gfx);

      m_Impl->m_Selection = new MouseSelectionFilter(this);
      m_Impl->m_SelPainter = new SelectionPainter(this);

      QObject::connect(m_Impl->m_Selection, &MouseSelectionFilter::onSelectionChanged, [this](AABB2Di const& iSel)
      {
        m_Impl->m_CurrentSelection = iSel;
        m_Impl->RefreshTilesFromSelection();
      });

      QObject::connect(m_Impl->m_Selection, &MouseSelectionFilter::onSelectionEnded, [this](AABB2Di const& iSel)
      {
        m_Impl->m_CurrentSelection = iSel;
        m_Impl->RefreshTilesFromSelection();
      });

      m_Impl->m_MainImageView->installEventFilter(m_Impl->m_Selection);
      m_Impl->m_MainImageView->SetPainterInterface(m_Impl->m_SelPainter);
      
      GfxSystem::ViewInfo& view = m_Impl->m_MainImageView->GetViewInfo();
      view.pos = Vector3f::UNIT_Z;
      view.projection = GfxSystem::Orthographic;
      view.displayedSize = 1.0;
      view.backgroundColor = Vector4f::ONE;
      
      m_Impl->m_Gfx->SetView(view);
      
      m_Impl->m_ImageHandle = m_Impl->m_World.CreateObject();
      
      m_Impl->m_Transforms->AddTransform(m_Impl->m_ImageHandle, nullptr);

      GfxComponent& gfxComp = m_Impl->m_Gfx->CreateComponent(m_Impl->m_ImageHandle);

      //m_Impl->m_RatioWidgetImage = new AspectRatioWidget(m_Impl->m_MainImageView, 1.0, 1.0, this);

      QWidget* imgDisplayWidget = new QWidget(this);
      QVBoxLayout* imgDisplayLayout = new QVBoxLayout(imgDisplayWidget);
      imgDisplayWidget->setLayout(imgDisplayLayout);
      
      m_Impl->m_Toolbox = new TilesetToolboxWidget(this);
      
      QObject::connect(m_Impl->m_Toolbox->m_Ui.gridSizeX, QOverload<int>::of(&QSpinBox::valueChanged), [this](int i)
      {
        m_Impl->m_GridSize.SetX(i);
        m_Impl->RefreshTilesFromSelection();
      });
      QObject::connect(m_Impl->m_Toolbox->m_Ui.gridSizeY, QOverload<int>::of(&QSpinBox::valueChanged), [this](int i)
      {
        m_Impl->m_GridSize.SetY(i);
        m_Impl->RefreshTilesFromSelection();
      });
      QObject::connect(m_Impl->m_Toolbox->m_Ui.offsetX, QOverload<int>::of(&QSpinBox::valueChanged), [this](int i)
      {
        m_Impl->m_SelOffset.SetX(i);
        m_Impl->RefreshTilesFromSelection();
      });
      QObject::connect(m_Impl->m_Toolbox->m_Ui.offsetY, QOverload<int>::of(&QSpinBox::valueChanged), [this](int i)
      {
        m_Impl->m_SelOffset.SetY(i);
        m_Impl->RefreshTilesFromSelection();
      });
      QObject::connect(m_Impl->m_Toolbox->m_Ui.bootstrapButton, &QAbstractButton::pressed, [this]()
      {
        m_Impl->Bootstrap();
      });

      imgDisplayLayout->addWidget(m_Impl->m_Toolbox);

      QScrollArea* imgScroll = new QScrollArea(this);
      imgScroll->setWidget(m_Impl->m_MainImageView);

      imgDisplayLayout->addWidget(imgScroll);
      viewSplitter->addWidget(imgDisplayWidget);
    }

    {
      m_Impl->m_TilePreview = new GameWidget(this);
      m_Impl->m_TilePreview->SetGfxSystem(m_Impl->m_Gfx);

      m_Impl->m_GridPainter = new GridPainter(m_Impl->m_TilePreview);
      m_Impl->m_TilePreview->SetPainterInterface(m_Impl->m_GridPainter);

      GfxSystem::ViewInfo& view = m_Impl->m_TilePreview->GetViewInfo();
      view.pos = Vector3f::UNIT_Z + Vector3f::UNIT_X * 2;
      view.projection = GfxSystem::Orthographic;
      view.displayedSize = 1.0;
      view.backgroundColor = Vector4f::ONE;
      m_Impl->m_TilePreview->ViewInfoUpdated();

      m_Impl->m_Gfx->SetView(view);

      m_Impl->m_AnimHandle = m_Impl->m_World.CreateObject();

      Matrix4f animPosition = Matrix4f::IDENTITY;
      MathTools::GetPosition2D(animPosition) = Vector2f::UNIT_X * 2;

      m_Impl->m_Transforms->AddTransform(m_Impl->m_AnimHandle, &animPosition);
      GfxSpriteComponent& gfxComp = m_Impl->m_Gfx->CreateSpriteComponent(m_Impl->m_AnimHandle);
      gfxComp.SetTileset(m_Impl->m_Tileset);

      m_Impl->m_RatioWidgetTile = new AspectRatioWidget(m_Impl->m_TilePreview, 1.0, 1.0, this);

      viewSplitter->addWidget(m_Impl->m_RatioWidgetTile);
      m_Impl->m_TilePreview->SetAnimated(true);
    }
		
		rootSplitter->addWidget(dataSplitter);
		rootSplitter->addWidget(viewSplitter);

		QVBoxLayout* layout = new QVBoxLayout(this);

		layout->addWidget(rootSplitter);

		setLayout(layout);
	}

  void TilesetEditor::Impl::UpdateTile(QModelIndex iIndex)
  {
    if (iIndex.isValid())
    {
      Tile const* tile = m_TileCollectionModel->GetTileFromIndex(iIndex);
      eXl_ASSERT(tile != nullptr);

      m_CurrentEditedTile = *tile;

      m_CurrentDisplayedTile = *m_TileCollectionModel->GetTileNameFromIndex(iIndex);
      m_CurrentDisplayedImage = m_CurrentEditedTile.m_ImageName;

      ObjectModel::CreateOrUpdateModel(m_TileDataView, false, DynObject(Tile::GetType(), &m_CurrentEditedTile));

      auto onTileDataChanged = [this, iIndex]()
      {
        m_Editor->ModifyResource();
        if (!m_Tileset->AddTile(*m_CurrentDisplayedTile, m_CurrentEditedTile))
        {
          m_CurrentDisplayedImage = {};
        }
        else
        {
          m_CurrentDisplayedImage = m_CurrentEditedTile.m_ImageName;
        }

        UpdateImage();
        UpdateTilePreview(iIndex);
      };

      QObject::connect(m_TileDataView->model(), &QAbstractItemModel::dataChanged, [onTileDataChanged](QModelIndex const& iDataIndex)
      {
        onTileDataChanged();
      });

      QObject::connect(m_TileDataView->model(), &QAbstractItemModel::rowsInserted, onTileDataChanged);
      QObject::connect(m_TileDataView->model(), &QAbstractItemModel::rowsRemoved, onTileDataChanged);

      UpdateImage();
      UpdateTilePreview(iIndex);
    }
  }

  void TilesetEditor::Impl::UpdateTilePreview(QModelIndex iIndex)
  {
    GfxSpriteComponent* comp = m_Gfx->GetSpriteComponent(m_AnimHandle);
    
    TileName const* name = m_TileCollectionModel->GetTileNameFromIndex(iIndex);
    if (name)
    {
      Tile const* tile = m_Tileset->Find(*name);
      comp->SetTileName(*name);

      float aspectRatio = float(tile->m_Size.Y()) / tile->m_Size.X();
      float maxDim = Mathf::Max(tile->m_Size.X() * tile->m_Scale.X() / EngineCommon::s_WorldToPixel + 2 * Mathf::Abs(tile->m_Offset.X())
        , tile->m_Size.Y() * tile->m_Scale.Y() / EngineCommon::s_WorldToPixel + 2 * Mathf::Abs(tile->m_Offset.Y()));

      GfxSystem::ViewInfo& view = m_TilePreview->GetViewInfo();
      view.displayedSize = maxDim * 1.2;
      m_Gfx->SetView(view);
      m_TilePreview->ViewInfoUpdated();

      m_RatioWidgetTile->SetAspectRatio(tile->m_Size.X(), tile->m_Size.Y());
    }
    else
    {
      comp->SetTileName(TileName(""));
    }
  }

  void TilesetEditor::Impl::RefreshSelectionBox()
  {
    m_SelPainter->m_Boxes.clear();

    if (!m_CurrentDisplayedImage)
    {
      return;
    }
    for (auto const& boxOrig : m_CurBoxes)
    {
      AABB2Di box(boxOrig, m_GridSize);
      m_SelPainter->m_Boxes.push_back(box);
    }
   
    m_MainImageView->update();
  }

  void TilesetEditor::Impl::RefreshTilesFromSelection()
  {
    m_CurBoxes.clear();
    Vector2i size = m_CurrentSelection.GetSize();
    if (size != Vector2i::ZERO)
    {
      AABB2Di croppedBox;
      Vector2i imageSize = m_Tileset->GetImageSize(*m_CurrentDisplayedImage);
      croppedBox.SetCommonBox(m_CurrentSelection, AABB2Di(Vector2i::ZERO, imageSize * 2));

      AABB2Di snapBox;
      snapBox.m_Data[0].X() = (croppedBox.m_Data[0].X() - m_SelOffset.X() + m_GridSize.X() / 2) / m_GridSize.X();
      snapBox.m_Data[0].Y() = (croppedBox.m_Data[0].Y() - m_SelOffset.Y() + m_GridSize.Y() / 2) / m_GridSize.Y();
      snapBox.m_Data[1].X() = (croppedBox.m_Data[1].X() - m_SelOffset.X() + m_GridSize.X() / 2) / m_GridSize.X();
      snapBox.m_Data[1].Y() = (croppedBox.m_Data[1].Y() - m_SelOffset.Y() + m_GridSize.Y() / 2) / m_GridSize.Y();

      snapBox.SetCommonBox(snapBox, AABB2Di(Vector2i::ZERO, imageSize * 2));

      Vector2i snapSize = snapBox.GetSize();
      snapSize.X() = Mathi::Max(snapSize.X(), 1);
      snapSize.Y() = Mathi::Max(snapSize.Y(), 1);

      for (int32_t i = 0; i < snapSize.Y(); ++i)
      {
        for (int32_t j = 0; j < snapSize.X(); ++j)
        {
          m_CurBoxes.push_back(Vector2i((snapBox.m_Data[0].X() + j) * m_GridSize.X() + m_SelOffset.X(), (snapBox.m_Data[0].Y() + i) * m_GridSize.Y() + m_SelOffset.Y()));
        }
      }
    }

    RefreshSelectionBox();
  }

  void TilesetEditor::Impl::UpdateImage()
  {
    GfxComponent* gfxComp = m_Gfx->GetComponent(m_ImageHandle);
    gfxComp->ClearDraws();

    if (m_CurrentDisplayedImage)
    {
      Vector2i imageSize = m_Tileset->GetImageSize(*m_CurrentDisplayedImage);
      float aspectRatio = float(imageSize.Y()) / imageSize.X();

      IntrusivePtr<GeometryInfo> geom(eXl_NEW GeometryInfo);
      geom->m_Vertices = GfxSpriteData::MakeSpriteGeometry(Vector2f(1.0 / aspectRatio, 1.0), true);

      unsigned int indexData[] = { 0, 1, 2, 2, 1, 3 };
      geom->m_Indices = OGLBuffer::CreateBuffer(OGLBufferUsage::ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData);
      geom->SetupAssembly(true);
      geom->m_Command = OGLDraw::TriangleList;

      IntrusivePtr<SpriteMaterialInfo> matInfo(eXl_NEW SpriteMaterialInfo);
      matInfo->m_Texture = m_Tileset->GetTexture(*m_CurrentDisplayedImage);
      matInfo->m_SpriteInfo.tint = Vector4f::ONE;
      matInfo->m_SpriteInfo.alphaMult = 1.0;
      matInfo->m_SpriteInfo.tcScaling = Vector2f::ONE;
      matInfo->m_SpriteInfo.imageSize = MathTools::ToFVec(imageSize);
      matInfo->SetupData();

      gfxComp->SetGeometry(geom.get());
      gfxComp->AddDraw(matInfo.get(), 6, 0);

      m_MainImageView->setFixedSize(QSize(imageSize.X(), imageSize.Y()));
      //m_RatioWidgetImage->SetAspectRatio(imageSize.X(), imageSize.Y());
      //m_RatioWidgetImage->setFixedSize(QSize(imageSize.X(), imageSize.Y()));
    }
  }

  void TilesetEditor::Impl::Bootstrap()
  {
    if (m_CurrentDisplayedImage)
    {
      if (Image const* img = m_Tileset->GetImage(*m_CurrentDisplayedImage))
      {
        uint32_t pixelSize = img->GetPixelSize();
        Vector<uint8_t> pixel(pixelSize);
        Vector2i gridSize(img->GetSize().X() / m_GridSize.X(), img->GetSize().Y() / m_GridSize.Y());
        for (int32_t y = 0; y < gridSize.Y(); ++y)
        {
          uint8_t const* row = (uint8_t const*)img->GetRow(y * m_GridSize.Y());
          for (int32_t x = 0; x < gridSize.X(); ++x)
          {
            Vector2i offset(x * m_GridSize.X(), y * m_GridSize.Y());
            bool validTile = false;
            for (int32_t sY = 0; sY < m_GridSize.Y(); ++sY)
            {
              uint8_t const* subRow = row + sY * img->GetRowStride();
              for (int32_t sX = 0; !validTile && sX < m_GridSize.X(); ++sX)
              {
                if (sY == 0 && sX == 0)
                {
                  memcpy(pixel.data(), subRow, pixel.size());
                }
                else
                {
                  if (memcmp(pixel.data(), subRow, pixel.size()) != 0)
                  {
                    validTile = true;
                  }
                }
                subRow += pixelSize;
              }
            }

            if (validTile)
            {
              uint32_t curRowCount = m_TileCollectionModel->rowCount(QModelIndex());
              Tile newTile;
              newTile.m_ImageName = *m_CurrentDisplayedImage;
              newTile.m_Size = m_GridSize;
              newTile.m_Frames.push_back(offset);
              TileName newName(String("Tile_") + StringUtil::FromInt(x) + "_" + StringUtil::FromInt(y));
              m_TileCollectionModel->AddTile(newName, newTile);
            }

            row += m_GridSize.X() * pixelSize;
          }
        }
      }
    }
  }
}