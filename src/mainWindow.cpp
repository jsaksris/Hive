/*
* Copyright (C) 2017-2019, Emilien Vallot, Christophe Calmejane and other contributors

* This file is part of Hive.

* Hive is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* Hive is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with Hive.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mainWindow.hpp"
#include <QtWidgets>
#include <QMessageBox>
#include <QFile>
#include <QTextBrowser>
#include <QDateTime>
#include <QAbstractListModel>
#include <QVector>

#ifdef DEBUG
#	include <QFileInfo>
#	include <QDir>
#endif

#include "avdecc/helper.hpp"
#include "avdecc/hiveLogItems.hpp"
#include "avdecc/channelConnectionManager.hpp"
#include "internals/config.hpp"

#include "nodeVisitor.hpp"
#include "toolkit/dynamicHeaderView.hpp"
#include "toolkit/material/colorPalette.hpp"
#include "avdecc/controllerManager.hpp"
#include "avdecc/mcDomainManager.hpp"
#include "aboutDialog.hpp"
#include "settingsDialog.hpp"
#include "mediaClock/mediaClockManagementDialog.hpp"
#include "highlightForegroundItemDelegate.hpp"
#include "imageItemDelegate.hpp"
#include "settingsManager/settings.hpp"
#include "entityLogoCache.hpp"
#include "deviceDetailsDialog.hpp"

#include "updater/updater.hpp"

#include <la/avdecc/networkInterfaceHelper.hpp>

#include <mutex>
#include <memory>

extern "C"
{
#include <mkdio.h>
}

#define VENDOR_ID 0x001B92
#define DEVICE_ID 0x80
#define MODEL_ID 0x00000001

Q_DECLARE_METATYPE(la::avdecc::protocol::ProtocolInterface::Type)

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, _controllerModel(new avdecc::ControllerModel(this))
{
	// Setup common UI
	setupUi(this);

	// Set title
	setWindowTitle(hive::internals::applicationLongName + " - Version " + QCoreApplication::applicationVersion());

	// Register all Qt metatypes
	registerMetaTypes();

	// Setup the current profile
	setupProfile();
}

void MainWindow::setupAdvancedView(Defaults const& defaults)
{
	// Create "view" sub-menu
	createViewMenu();

	// Create the main toolbar
	createMainToolBar();

	// Create the ControllerView widget
	createControllerView();

	// Initialize the Interface ComboBox
	initInterfaceComboBox();

	// Initialize UI defaults
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::EntityLogo), !defaults.controllerTableView_EntityLogo_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::Compatibility), !defaults.controllerTableView_Compatibility_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::Name), !defaults.controllerTableView_Name_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::Group), !defaults.controllerTableView_Group_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::AcquireState), !defaults.controllerTableView_AcquireState_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::LockState), !defaults.controllerTableView_LockState_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::GrandmasterId), !defaults.controllerTableView_GrandmasterID_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::GptpDomain), !defaults.controllerTableView_GptpDomain_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::InterfaceIndex), !defaults.controllerTableView_InterfaceIndex_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::AssociationId), !defaults.controllerTableView_AssociationID_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::MediaClockMasterId), !defaults.controllerTableView_MediaClockMasterID_Visible);
	controllerTableView->setColumnHidden(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::MediaClockMasterName), !defaults.controllerTableView_MediaClockMasterName_Visible);

	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::EntityLogo), defaults.ColumnWidth_Logo);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::Compatibility), defaults.ColumnWidth_Compatibility);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::EntityId), defaults.ColumnWidth_UniqueIdentifier);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::Name), defaults.ColumnWidth_Name);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::Group), defaults.ColumnWidth_Group);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::AcquireState), defaults.ColumnWidth_ExclusiveAccessState);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::LockState), defaults.ColumnWidth_ExclusiveAccessState);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::GrandmasterId), defaults.ColumnWidth_UniqueIdentifier);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::GptpDomain), defaults.ColumnWidth_GPTPDomain);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::InterfaceIndex), defaults.ColumnWidth_InterfaceIndex);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::AssociationId), defaults.ColumnWidth_UniqueIdentifier);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::MediaClockMasterId), defaults.ColumnWidth_UniqueIdentifier);
	controllerTableView->setColumnWidth(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::MediaClockMasterName), defaults.ColumnWidth_Name);

	mainToolBar->setVisible(defaults.mainWindow_Toolbar_Visible);
	entityInspectorDockWidget->setVisible(defaults.mainWindow_Inspector_Visible);
	loggerDockWidget->setVisible(defaults.mainWindow_Logger_Visible);

	// Load settings, overriding defaults
	loadSettings();

	// Connect all signals
	connectSignals();

	// Register AcceptDrops so we can drop VirtualEntities as JSON
	setAcceptDrops(true);

	// Create channel connection manager instance
	avdecc::ChannelConnectionManager::getInstance();
}

void MainWindow::setupStandardProfile()
{
	setupAdvancedView(Defaults{ true, false, false, true, true, true, true, false, false, false, false, false, false, true, true });
}

void MainWindow::setupDeveloperProfile()
{
	setupAdvancedView(Defaults{});
}

void MainWindow::setupProfile()
{
	// Update the UI and other stuff, based on the current Profile
	auto& settings = settings::SettingsManager::getInstance();

	auto const userProfile = settings.getValue(settings::UserProfile.name).value<profiles::ProfileType>();

	switch (userProfile)
	{
		case profiles::ProfileType::None:
		default:
			AVDECC_ASSERT(false, "No profile selected");
			[[fallthrough]];
		case profiles::ProfileType::Standard:
			setupStandardProfile();
			break;
		case profiles::ProfileType::Developer:
			setupDeveloperProfile();
			break;
	}
}

void MainWindow::currentControllerChanged()
{
	auto& settings = settings::SettingsManager::getInstance();

	auto const protocolType = settings.getValue(settings::ProtocolType.name).value<la::avdecc::protocol::ProtocolInterface::Type>();
	auto const interfaceID = _interfaceComboBox.currentData().toString();
	if (interfaceID.isEmpty())
	{
		LOG_HIVE_WARN("No Network Interface selected. Please choose one.");
		return;
	}

	settings.setValue(settings::InterfaceID, interfaceID);

	try
	{
		// Create a new Controller
		auto& manager = avdecc::ControllerManager::getInstance();
		manager.createController(protocolType, interfaceID, 0x0003, la::avdecc::entity::model::makeEntityModelID(VENDOR_ID, DEVICE_ID, MODEL_ID), "en");
		_controllerEntityIDLabel.setText(avdecc::helper::uniqueIdentifierToString(manager.getControllerEID()));
	}
	catch (la::avdecc::controller::Controller::Exception const& e)
	{
		LOG_HIVE_WARN(e.what());
	}
}

void MainWindow::currentControlledEntityChanged(QModelIndex const& index)
{
	if (!index.isValid())
	{
		entityInspector->setControlledEntityID(la::avdecc::UniqueIdentifier{});
		return;
	}

	auto& manager = avdecc::ControllerManager::getInstance();
	auto const entityID = _controllerModel->controlledEntityID(index);
	auto controlledEntity = manager.getControlledEntity(entityID);

	if (controlledEntity)
	{
		entityInspector->setControlledEntityID(entityID);
	}
}

// Private methods
void MainWindow::registerMetaTypes()
{
	//
	qRegisterMetaType<la::avdecc::logger::Layer>("la::avdecc::logger::Layer");
	qRegisterMetaType<la::avdecc::logger::Level>("la::avdecc::logger::Level");
	qRegisterMetaType<std::string>("std::string");

	qRegisterMetaType<EntityLogoCache::Type>("EntityLogoCache::Type");
}

void MainWindow::createViewMenu()
{
	menuView->addAction(mainToolBar->toggleViewAction());
	menuView->addSeparator();
	menuView->addAction(entityInspectorDockWidget->toggleViewAction());
	menuView->addSeparator();
	menuView->addAction(loggerDockWidget->toggleViewAction());
}

void MainWindow::createMainToolBar()
{
	auto* interfaceLabel = new QLabel("Interface");
	interfaceLabel->setMinimumWidth(50);
	_interfaceComboBox.setMinimumWidth(100);

	_refreshControllerButton.setToolTip("Reload Controller");

	auto* controllerEntityIDLabel = new QLabel("Controller ID: ");
	controllerEntityIDLabel->setMinimumWidth(50);
	_controllerEntityIDLabel.setMinimumWidth(100);

	mainToolBar->setMinimumHeight(30);

	mainToolBar->addWidget(interfaceLabel);
	mainToolBar->addWidget(&_interfaceComboBox);

	mainToolBar->addSeparator();

	mainToolBar->addWidget(&_refreshControllerButton);

	mainToolBar->addSeparator();

	mainToolBar->addWidget(controllerEntityIDLabel);
	mainToolBar->addWidget(&_controllerEntityIDLabel);

#ifdef Q_OS_MAC
	// See https://bugreports.qt.io/browse/QTBUG-13635
	mainToolBar->setStyleSheet("QToolBar QLabel { padding-bottom: 5; }");
#endif
}

void MainWindow::createControllerView()
{
	controllerTableView->setModel(_controllerModel);
	controllerTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	controllerTableView->setSelectionMode(QAbstractItemView::SingleSelection);
	controllerTableView->setContextMenuPolicy(Qt::CustomContextMenu);
	controllerTableView->setFocusPolicy(Qt::ClickFocus);

	// Disable row resizing
	controllerTableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

	auto* imageItemDelegate{ new ImageItemDelegate{ this } };
	controllerTableView->setItemDelegateForColumn(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::EntityLogo), imageItemDelegate);
	controllerTableView->setItemDelegateForColumn(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::Compatibility), imageItemDelegate);
	controllerTableView->setItemDelegateForColumn(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::AcquireState), imageItemDelegate);
	controllerTableView->setItemDelegateForColumn(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::LockState), imageItemDelegate);

	auto* highlightForegroundItemDelegate{ new HighlightForegroundItemDelegate{ this } };
	controllerTableView->setItemDelegateForColumn(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::EntityId), highlightForegroundItemDelegate);

	_controllerDynamicHeaderView.setHighlightSections(false);
	_controllerDynamicHeaderView.setMandatorySection(la::avdecc::utils::to_integral(avdecc::ControllerModel::Column::EntityId));
	controllerTableView->setHorizontalHeader(&_controllerDynamicHeaderView);
}

void MainWindow::initInterfaceComboBox()
{
	_networkInterfaceModelProxy.setSourceModel(&_networkInterfaceModel);
	_networkInterfaceModelProxy.setSortRole(Qt::UserRole);
	_networkInterfaceModelProxy.sort(0, Qt::AscendingOrder);

	_interfaceComboBox.setModel(&_networkInterfaceModelProxy);
}

void MainWindow::loadSettings()
{
	auto& settings = settings::SettingsManager::getInstance();

	LOG_HIVE_DEBUG("Settings location: " + settings.getFilePath());

	auto const networkInterfaceIndex = _networkInterfaceModel.indexOf(settings.getValue(settings::InterfaceID).toString().toStdString());
	if (!networkInterfaceIndex.isValid() || !_networkInterfaceModel.isEnabled(networkInterfaceIndex))
	{
		_interfaceComboBox.setCurrentIndex(-1);
	}
	else
	{
		_interfaceComboBox.setCurrentIndex(_networkInterfaceModelProxy.mapFromSource(networkInterfaceIndex).row());
	}

	// Check if currently saved ProtocolInterface is supported
	auto protocolType = settings.getValue(settings::ProtocolType.name).value<la::avdecc::protocol::ProtocolInterface::Type>();
	auto const supportedTypes = la::avdecc::protocol::ProtocolInterface::getSupportedProtocolInterfaceTypes();
	if (!supportedTypes.test(protocolType) && !supportedTypes.empty())
	{
		// Force the first supported ProtocolInterface, and save it to the settings, before we call registerSettingObserver
		protocolType = *supportedTypes.begin();
		settings.setValue(settings::ProtocolType.name, la::avdecc::utils::to_integral(protocolType));
	}

	_controllerDynamicHeaderView.restoreState(settings.getValue(settings::ControllerDynamicHeaderViewState).toByteArray());
	loggerView->header()->restoreState(settings.getValue(settings::LoggerDynamicHeaderViewState).toByteArray());
	entityInspector->restoreState(settings.getValue(settings::EntityInspectorState).toByteArray());
	splitter->restoreState(settings.getValue(settings::SplitterState).toByteArray());

	restoreGeometry(settings.getValue(settings::MainWindowGeometry).toByteArray());
	restoreState(settings.getValue(settings::MainWindowState).toByteArray());

	// Configure settings observers
	settings.registerSettingObserver(settings::ProtocolType.name, this);
	settings.registerSettingObserver(settings::ThemeColorIndex.name, this);
}

void MainWindow::connectSignals()
{
	connect(&_interfaceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::currentControllerChanged);
	connect(&_refreshControllerButton, &QPushButton::clicked, this, &MainWindow::currentControllerChanged);

	connect(controllerTableView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::currentControlledEntityChanged);
	connect(&_controllerDynamicHeaderView, &qt::toolkit::DynamicHeaderView::sectionChanged, this,
		[this]()
		{
			auto& settings = settings::SettingsManager::getInstance();
			settings.setValue(settings::ControllerDynamicHeaderViewState, _controllerDynamicHeaderView.saveState());
		});

	connect(controllerTableView, &QTableView::doubleClicked, this,
		[this](QModelIndex const& index)
		{
			auto& manager = avdecc::ControllerManager::getInstance();
			auto const entityID = _controllerModel->controlledEntityID(index);
			auto controlledEntity = manager.getControlledEntity(entityID);

			auto const& entity = controlledEntity->getEntity();
			if (controlledEntity->getEntity().getEntityCapabilities().test(la::avdecc::entity::EntityCapability::AemSupported))
			{
				DeviceDetailsDialog* dialog = new DeviceDetailsDialog(this);
				dialog->setControlledEntityID(entityID);
				dialog->show();
				connect(dialog, &DeviceDetailsDialog::finished, this,
					[this, dialog](int result)
					{
						dialog->deleteLater();
					});
			}
		});

	connect(controllerTableView, &QTableView::customContextMenuRequested, this,
		[this](QPoint const& pos)
		{
			auto const index = controllerTableView->indexAt(pos);

			auto& manager = avdecc::ControllerManager::getInstance();
			auto const entityID = _controllerModel->controlledEntityID(index);
			auto controlledEntity = manager.getControlledEntity(entityID);

			if (controlledEntity)
			{
				QMenu menu;
				auto const& entity = controlledEntity->getEntity();

				auto* acquireAction{ static_cast<QAction*>(nullptr) };
				auto* releaseAction{ static_cast<QAction*>(nullptr) };
				auto* lockAction{ static_cast<QAction*>(nullptr) };
				auto* unlockAction{ static_cast<QAction*>(nullptr) };
				auto* deviceView{ static_cast<QAction*>(nullptr) };
				auto* inspect{ static_cast<QAction*>(nullptr) };
				auto* getLogo{ static_cast<QAction*>(nullptr) };
				auto* clearErrorFlags{ static_cast<QAction*>(nullptr) };
				auto* dumpEntity{ static_cast<QAction*>(nullptr) };

				if (entity.getEntityCapabilities().test(la::avdecc::entity::EntityCapability::AemSupported))
				{
					// Do not propose Acquire if the device is Milan (not supported)
					if (!controlledEntity->getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::Milan))
					{
						QString acquireText;
						auto const isAcquired = controlledEntity->isAcquired();
						auto const isAcquiredByOther = controlledEntity->isAcquiredByOther();

						{
							if (isAcquiredByOther)
								acquireText = "Try to acquire";
							else
								acquireText = "Acquire";
							acquireAction = menu.addAction(acquireText);
							acquireAction->setEnabled(!isAcquired);
						}
						{
							releaseAction = menu.addAction("Release");
							releaseAction->setEnabled(isAcquired);
						}
					}
					// Lock
					{
						QString lockText;
						auto const isLocked = controlledEntity->isLocked();
						auto const isLockedByOther = controlledEntity->isLockedByOther();

						{
							if (isLockedByOther)
								lockText = "Try to lock";
							else
								lockText = "Lock";
							lockAction = menu.addAction(lockText);
							lockAction->setEnabled(!isLocked);
						}
						{
							unlockAction = menu.addAction("Unlock");
							unlockAction->setEnabled(isLocked);
						}
					}

					menu.addSeparator();

					// Device Details, Inspect, Logo, ...
					{
						deviceView = menu.addAction("Device Details...");
					}
					{
						inspect = menu.addAction("Inspect Entity Model...");
					}
					{
						getLogo = menu.addAction("Retrieve Entity Logo");
						getLogo->setEnabled(!EntityLogoCache::getInstance().isImageInCache(entityID, EntityLogoCache::Type::Entity));
					}
					{
						clearErrorFlags = menu.addAction("Acknowledge Counters Errors");
					}
				}

				menu.addSeparator();

				// Dump Entity
				{
					dumpEntity = menu.addAction("Export Entity...");
				}

				menu.addSeparator();

				// Cancel
				menu.addAction("Cancel");

				// Release the controlled entity before starting a long operation (menu.exec)
				controlledEntity.reset();

				if (auto* action = menu.exec(controllerTableView->viewport()->mapToGlobal(pos)))
				{
					if (action == acquireAction)
					{
						manager.acquireEntity(entityID, false);
					}
					else if (action == releaseAction)
					{
						manager.releaseEntity(entityID);
					}
					else if (action == lockAction)
					{
						manager.lockEntity(entityID);
					}
					else if (action == unlockAction)
					{
						manager.unlockEntity(entityID);
					}
					else if (action == deviceView)
					{
						DeviceDetailsDialog* dialog = new DeviceDetailsDialog(this);
						dialog->setControlledEntityID(entityID);
						dialog->show();
						connect(dialog, &DeviceDetailsDialog::finished, this,
							[this, dialog](int result)
							{
								dialog->deleteLater();
							});
					}
					else if (action == inspect)
					{
						auto* inspector = new EntityInspector;
						inspector->setAttribute(Qt::WA_DeleteOnClose);
						inspector->setControlledEntityID(entityID);
						inspector->restoreGeometry(entityInspector->saveGeometry());
						inspector->show();
					}
					else if (action == getLogo)
					{
						EntityLogoCache::getInstance().getImage(entityID, EntityLogoCache::Type::Entity, true);
					}
					else if (action == clearErrorFlags)
					{
						manager.clearAllStreamInputCounterValidFlags(entityID);
					}
					else if (action == dumpEntity)
					{
						auto const filename = QFileDialog::getSaveFileName(this, "Save As...", QString("%1/Entity_%2.json").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(avdecc::helper::uniqueIdentifierToString(entityID)), "*.json");
						if (!filename.isEmpty())
						{
							auto [error, message] = manager.serializeControlledEntityAsReadableJson(entityID, filename, false);
							if (!error)
							{
								QMessageBox::information(this, "", "Export successfully completed:\n" + filename);
							}
							else
							{
								if (error == la::avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex)
								{
									auto const choice = QMessageBox::question(this, "", QString("EntityID %1 model is not fully IEEE1722.1 compliant.\n%2\n\nDo you want to export anyway?").arg(avdecc::helper::uniqueIdentifierToString(entityID)).arg(message.c_str()), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No);
									if (choice == QMessageBox::StandardButton::Yes)
									{
										auto const result = manager.serializeControlledEntityAsReadableJson(entityID, filename, true);
										error = std::get<0>(result);
										message = std::get<1>(result);
										if (!error)
										{
											QMessageBox::information(this, "", "Export completed but with warnings:\n" + filename);
										}
										// Fallthrough to warning message
									}
								}
								if (!!error)
								{
									QMessageBox::warning(this, "", QString("Export of EntityID %1 failed:\n%2").arg(avdecc::helper::uniqueIdentifierToString(entityID)).arg(message.c_str()));
								}
							}
						}
					}
				}
			}
		});

	connect(entityInspector, &EntityInspector::stateChanged, this,
		[this]()
		{
			auto& settings = settings::SettingsManager::getInstance();
			settings.setValue(settings::EntityInspectorState, entityInspector->saveState());
		});

	connect(loggerView->header(), &qt::toolkit::DynamicHeaderView::sectionChanged, this,
		[this]()
		{
			auto& settings = settings::SettingsManager::getInstance();
			settings.setValue(settings::LoggerDynamicHeaderViewState, loggerView->header()->saveState());
		});

	connect(splitter, &QSplitter::splitterMoved, this,
		[this]()
		{
			auto& settings = settings::SettingsManager::getInstance();
			settings.setValue(settings::SplitterState, splitter->saveState());
		});

	// Connect ControllerManager events
	auto& manager = avdecc::ControllerManager::getInstance();
	connect(&manager, &avdecc::ControllerManager::endAecpCommand, this,
		[this](la::avdecc::UniqueIdentifier const entityID, avdecc::ControllerManager::AecpCommandType commandType, la::avdecc::entity::ControllerEntity::AemCommandStatus const status)
		{
			if (status != la::avdecc::entity::ControllerEntity::AemCommandStatus::Success)
			{
				QMessageBox::warning(this, "", "<i>" + avdecc::ControllerManager::typeToString(commandType) + "</i> failed:<br>" + QString::fromStdString(la::avdecc::entity::ControllerEntity::statusToString(status)));
			}
		});
	connect(&manager, &avdecc::ControllerManager::endAcmpCommand, this,
		[this](la::avdecc::UniqueIdentifier const talkerEntityID, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, avdecc::ControllerManager::AcmpCommandType commandType, la::avdecc::entity::ControllerEntity::ControlStatus const status)
		{
			if (status != la::avdecc::entity::ControllerEntity::ControlStatus::Success)
			{
				QMessageBox::warning(this, "", "<i>" + avdecc::ControllerManager::typeToString(commandType) + "</i> failed:<br>" + QString::fromStdString(la::avdecc::entity::ControllerEntity::statusToString(status)));
			}
		});

	//

	connect(actionExportFullNetworkState, &QAction::triggered, this,
		[this]()
		{
			auto const filename = QFileDialog::getSaveFileName(this, "Save As...", QString("%1/FullDump_%2.json").arg(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")), "*.json");
			if (!filename.isEmpty())
			{
				auto& manager = avdecc::ControllerManager::getInstance();
				auto [error, message] = manager.serializeAllControlledEntitiesAsReadableJson(filename, false);
				if (!error)
				{
					QMessageBox::information(this, "", "Export successfully completed:\n" + filename);
				}
				else
				{
					QMessageBox::warning(this, "", QString("Export failed:\n%1").arg(message.c_str()));
				}
			}
		});

	//

	connect(actionSettings, &QAction::triggered, this,
		[this]()
		{
			SettingsDialog dialog{ this };
			dialog.exec();
		});

	connect(actionMediaClockManagement, &QAction::triggered, this,
		[this]()
		{
			MediaClockManagementDialog dialog{ this };
			dialog.exec();
		});

	//

	connect(actionAbout, &QAction::triggered, this,
		[this]()
		{
			AboutDialog dialog{ this };
			dialog.exec();
		});

	//

	connect(actionChangeLog, &QAction::triggered, this,
		[this]()
		{
			showChangeLog("Change Log", "");
		});

	//

	connect(actionOpenProjectWebPage, &QAction::triggered, this,
		[]()
		{
			QDesktopServices::openUrl(hive::internals::projectURL);
		});

	// Connect updater signals
	auto const& updater = Updater::getInstance();
	connect(&updater, &Updater::newReleaseVersionAvailable, this,
		[](QString version, QString downloadURL)
		{
			QString message{ "New version (" + version + ") available.\nDo you want to open the download page?" };

			auto const result = QMessageBox::information(nullptr, "", message, QMessageBox::StandardButton::Open, QMessageBox::StandardButton::Cancel);
			if (result == QMessageBox::StandardButton::Open)
			{
				QDesktopServices::openUrl(downloadURL);
			}
			LOG_HIVE_INFO(message);
		});
	connect(&updater, &Updater::newBetaVersionAvailable, this,
		[](QString version, QString downloadURL)
		{
			QString message{ "New BETA version (" + version + ") available.\nDo you want to open the download page?" };

			auto const result = QMessageBox::information(nullptr, "", message, QMessageBox::StandardButton::Open, QMessageBox::StandardButton::Cancel);
			if (result == QMessageBox::StandardButton::Open)
			{
				QDesktopServices::openUrl(downloadURL);
			}
			LOG_HIVE_INFO(message);
		});
	connect(&updater, &Updater::checkFailed, this,
		[](QString reason)
		{
			LOG_HIVE_WARN("Failed to check for new version: " + reason);
		});

	auto* refreshController = new QShortcut{ QKeySequence{ "Ctrl+R" }, this };
	connect(refreshController, &QShortcut::activated, this, &MainWindow::currentControllerChanged);

#ifdef DEBUG
	auto* reloadStyleSheet = new QShortcut{ QKeySequence{ "F5" }, this };
	connect(reloadStyleSheet, &QShortcut::activated, this,
		[this]()
		{
			auto& settings = settings::SettingsManager::getInstance();
			auto const themeColorIndex = settings.getValue(settings::ThemeColorIndex.name).toInt();
			auto const colorName = qt::toolkit::material::color::Palette::name(themeColorIndex);
			updateStyleSheet(colorName, QString{ RESOURCES_ROOT_DIR } + "/style.qss");
			LOG_HIVE_DEBUG("StyleSheet reloaded");
		});
#endif
}

void MainWindow::showChangeLog(QString const title, QString const versionString)
{
	// Create dialog popup
	QDialog dialog{ this };
	QVBoxLayout layout{ &dialog };
	QTextBrowser view;
	layout.addWidget(&view);
	dialog.setWindowTitle(hive::internals::applicationShortName + " - " + title);
	dialog.resize(800, 600);
	QPushButton closeButton{ "Close" };
	connect(&closeButton, &QPushButton::clicked, &dialog,
		[&dialog]()
		{
			dialog.accept();
		});
	layout.addWidget(&closeButton);

	view.setContextMenuPolicy(Qt::NoContextMenu);
	view.setOpenExternalLinks(true);
	QFile changelogFile(":/CHANGELOG.md");
	if (changelogFile.open(QIODevice::ReadOnly))
	{
		auto content = QString(changelogFile.readAll());

		auto const startPos = content.indexOf("## [");
		auto endPos = versionString.isEmpty() ? -1 : content.indexOf("## [" + versionString + "]");
		if (endPos == -1)
			endPos = content.size();
		auto const changelog = QStringRef(&content, startPos, endPos - startPos);

		auto buffer = changelog.toUtf8();
		auto* mmiot = mkd_string(buffer.data(), buffer.size(), 0);
		if (mmiot == nullptr)
			return;
		std::unique_ptr<MMIOT, std::function<void(MMIOT*)>> scopedMmiot{ mmiot, [](MMIOT* ptr)
			{
				if (ptr != nullptr)
					mkd_cleanup(ptr);
			} };

		if (mkd_compile(mmiot, 0) == 0)
			return;

		char* docPointer{ nullptr };
		auto const docLength = mkd_document(mmiot, &docPointer);
		if (docLength == 0)
			return;

		view.setHtml(QString::fromUtf8(docPointer, docLength));

		// Run dialog
		dialog.exec();
	}
}

void MainWindow::showEvent(QShowEvent* event)
{
	QMainWindow::showEvent(event);

	static std::once_flag once;
	std::call_once(once,
		[this]()
		{
			auto& settings = settings::SettingsManager::getInstance();

			// Time to check for new version
			{
				auto& updater = Updater::getInstance();
				if (updater.isAutomaticCheckForNewVersion())
				{
					updater.checkForNewVersion();
				}
			}
			// Check if we have a network interface selected
			{
				auto const interfaceID = _interfaceComboBox.currentData().toString();
				if (interfaceID.isEmpty())
				{
					// Postpone the dialog creation
					QTimer::singleShot(0,
						[this]()
						{
							QMessageBox::warning(this, "", "No Network Interface selected.\nPlease choose one in the Toolbar.");
						});
				}
			}
			// Check if this is the first time we launch a new Hive version
			{
				auto lastVersion = settings.getValue(settings::LastLaunchedVersion.name).toString();
				settings.setValue(settings::LastLaunchedVersion.name, hive::internals::cmakeVersionString);

				// Do not show the ChangeLog during first ever launch, or if the last launched version is the same than current one
				if (lastVersion.isEmpty() || lastVersion == hive::internals::cmakeVersionString)
					return;

				// Postpone the dialog creation
				QTimer::singleShot(0,
					[this, versionString = std::move(lastVersion)]()
					{
						showChangeLog("What's New", versionString);
					});
			}
		});
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	auto& settings = settings::SettingsManager::getInstance();

	// Save window geometry
	settings.setValue(settings::MainWindowGeometry, saveGeometry());
	settings.setValue(settings::MainWindowState, saveState());

	// Unregister from settings
	settings.unregisterSettingObserver(settings::ProtocolType.name, this);
	settings.unregisterSettingObserver(settings::ThemeColorIndex.name, this);

	qApp->closeAllWindows();

	QMainWindow::closeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
	for (auto const& u : event->mimeData()->urls())
	{
		auto const f = QFileInfo{ u.fileName() };
		auto const ext = f.suffix();
		if (ext == "json")
		{
			event->acceptProposedAction();
			return;
		}
	}
}

void MainWindow::dropEvent(QDropEvent* event)
{
	auto& manager = avdecc::ControllerManager::getInstance();

	auto const loadEntity = [&manager](auto const& filePath, auto const ignoreSanityChecks)
	{
		auto const [error, message] = manager.loadVirtualEntityFromReadableJson(filePath, ignoreSanityChecks);
		auto msg = QString{};
		if (!!error)
		{
			switch (error)
			{
				case la::avdecc::jsonSerializer::DeserializationError::AccessDenied:
					msg = "Access Denied";
					break;
				case la::avdecc::jsonSerializer::DeserializationError::UnsupportedDumpVersion:
					msg = "Unsupported Dump Version";
					break;
				case la::avdecc::jsonSerializer::DeserializationError::ParseError:
					msg = QString("Parse Error: %1").arg(message.c_str());
					break;
				case la::avdecc::jsonSerializer::DeserializationError::MissingKey:
					msg = QString("Missing Key: %1").arg(message.c_str());
					break;
				case la::avdecc::jsonSerializer::DeserializationError::InvalidKey:
					msg = QString("Invalid Key: %1").arg(message.c_str());
					break;
				case la::avdecc::jsonSerializer::DeserializationError::InvalidValue:
					msg = QString("Invalid Value: %1").arg(message.c_str());
					break;
				case la::avdecc::jsonSerializer::DeserializationError::OtherError:
					msg = message.c_str();
					break;
				case la::avdecc::jsonSerializer::DeserializationError::DuplicateEntityID:
					msg = QString("An Entity already exists with the same EntityID: %1").arg(message.c_str());
					break;
				case la::avdecc::jsonSerializer::DeserializationError::NotCompliant:
					msg = message.c_str();
					break;
				case la::avdecc::jsonSerializer::DeserializationError::NotSupported:
					msg = "Virtual Entity Loading not supported by this version of the AVDECC library";
					break;
				case la::avdecc::jsonSerializer::DeserializationError::InternalError:
					msg = QString("Internal Error: %1").arg(message.c_str());
					break;
				default:
					AVDECC_ASSERT(false, "Unknown Error");
					msg = "Unknown Error";
					break;
			}
		}
		return std::make_tuple(error, msg);
	};

	for (auto const& u : event->mimeData()->urls())
	{
		auto const f = u.toLocalFile();
		auto const fi = QFileInfo{ f };
		auto const ext = fi.suffix();
		if (ext == "json")
		{
			auto [error, message] = loadEntity(u.toLocalFile(), false);
			if (!!error)
			{
				if (error == la::avdecc::jsonSerializer::DeserializationError::NotCompliant)
				{
					auto const choice = QMessageBox::question(this, "", "Entity model is not fully IEEE1722.1 compliant.\n\nDo you want to import anyway?", QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No);
					if (choice == QMessageBox::StandardButton::Yes)
					{
						auto const result = loadEntity(u.toLocalFile(), true);
						error = std::get<0>(result);
						message = std::get<1>(result);
						// Fallthrough to warning message
					}
				}
				if (!!error)
				{
					QMessageBox::warning(this, "Failed to load JSON entity", QString("Error loading JSON file '%1':\n%2").arg(f).arg(message));
				}
			}
		}
	}
}

void MainWindow::updateStyleSheet(qt::toolkit::material::color::Name const colorName, QString const& filename)
{
	auto const baseBackgroundColor = qt::toolkit::material::color::value(colorName);
	auto const baseForegroundColor = QColor{ qt::toolkit::material::color::luminance(colorName) == qt::toolkit::material::color::Luminance::Dark ? Qt::white : Qt::black };
	auto const connectionMatrixBackgroundColor = qt::toolkit::material::color::value(colorName, qt::toolkit::material::color::Shade::Shade100);

	// Load and apply the stylesheet
	auto styleFile = QFile{ filename };
	if (styleFile.open(QFile::ReadOnly))
	{
		auto const styleSheet = QString{ styleFile.readAll() }.arg(baseBackgroundColor.name()).arg(baseForegroundColor.name()).arg(connectionMatrixBackgroundColor.name());

		qApp->setStyleSheet(styleSheet);
	}
}

void MainWindow::onSettingChanged(settings::SettingsManager::Setting const& name, QVariant const& value) noexcept
{
	if (name == settings::ProtocolType.name)
	{
		currentControllerChanged();
	}
	else if (name == settings::ThemeColorIndex.name)
	{
		auto const colorName = qt::toolkit::material::color::Palette::name(value.toInt());
		updateStyleSheet(colorName, ":/style.qss");
	}
}
