// Copyright 2018 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Debugger/MemoryWidget.h"

#include <limits>
#include <optional>
#include <string>

#include <fmt/printf.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpacerItem>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

#include "Common/BitUtils.h"
#include "Common/FileUtil.h"
#include "Common/IOFile.h"
#include "Core/ConfigManager.h"
#include "Core/Core.h"
#include "Core/HW/AddressSpace.h"
#include "Core/PowerPC/PPCSymbolDB.h"
#include "Core/PowerPC/PowerPC.h"
#include "Core/System.h"
#include "DolphinQt/Debugger/MemoryViewWidget.h"
#include "DolphinQt/Host.h"
#include "DolphinQt/QtUtils/ModalMessageBox.h"
#include "DolphinQt/Settings.h"

using Type = MemoryViewWidget::Type;

static bool IsInstructionLoadStore(std::string_view ins)
{
  // Could add check for context address being near PC, because we need gprs to be correct for the
  // load/store.
  return (ins.starts_with('l') && !ins.starts_with("li")) || ins.starts_with("st") ||
         ins.starts_with("psq_l") || ins.starts_with("psq_s");
}

MemoryWidget::MemoryWidget(QWidget* parent) : QDockWidget(parent)
{
  setWindowTitle(tr("Memory"));
  setObjectName(QStringLiteral("memory"));

  setHidden(!Settings::Instance().IsMemoryVisible() || !Settings::Instance().IsDebugModeEnabled());

  setAllowedAreas(Qt::AllDockWidgetAreas);

  CreateWidgets();

  QSettings& settings = Settings::GetQSettings();

  restoreGeometry(settings.value(QStringLiteral("memorywidget/geometry")).toByteArray());
  // macOS: setHidden() needs to be evaluated before setFloating() for proper window presentation
  // according to Settings
  setFloating(settings.value(QStringLiteral("memorywidget/floating")).toBool());
  m_splitter->restoreState(settings.value(QStringLiteral("codewidget/splitter")).toByteArray());

  connect(&Settings::Instance(), &Settings::MemoryVisibilityChanged, this,
          [this](bool visible) { setHidden(!visible); });

  connect(&Settings::Instance(), &Settings::DebugModeToggled, this,
          [this](bool enabled) { setHidden(!enabled || !Settings::Instance().IsMemoryVisible()); });

  // Not really necessary
  // connect(&Settings::Instance(), &Settings::EmulationStateChanged, this, &MemoryWidget::Update);
  connect(Host::GetInstance(), &Host::UpdateDisasmDialog, this, &MemoryWidget::DisasmUpdate);

  LoadSettings();

  ConnectWidgets();
  OnAddressSpaceChanged();
  OnDisplayChanged();
}

MemoryWidget::~MemoryWidget()
{
  QSettings& settings = Settings::GetQSettings();

  settings.setValue(QStringLiteral("memorywidget/geometry"), saveGeometry());
  settings.setValue(QStringLiteral("memorywidget/floating"), isFloating());
  settings.setValue(QStringLiteral("memorywidget/splitter"), m_splitter->saveState());

  SaveSettings();
}

void MemoryWidget::CreateWidgets()
{
  auto* layout = new QHBoxLayout;

  layout->setContentsMargins(2, 2, 2, 2);
  layout->setSpacing(0);

  //// Sidebar

  // Search
  auto* m_address_splitter = new QSplitter(Qt::Horizontal);

  m_search_address = new QComboBox;
  m_search_address->setInsertPolicy(QComboBox::InsertAtTop);
  m_search_address->setDuplicatesEnabled(false);
  m_search_address->setEditable(true);
  m_search_address->lineEdit()->setPlaceholderText(tr("Search Address"));
  m_search_address->setMaxVisibleItems(8);
  m_search_offset = new QLineEdit;

  m_search_offset->setMaxLength(9);
  m_search_offset->setPlaceholderText(tr("Offset"));

  m_address_splitter->addWidget(m_search_address);
  m_address_splitter->addWidget(m_search_offset);
  m_address_splitter->setHandleWidth(1);
  m_address_splitter->setCollapsible(0, false);
  m_address_splitter->setStretchFactor(0, 3);
  m_address_splitter->setStretchFactor(1, 1);

  auto* input_layout = new QHBoxLayout;
  m_data_edit = new QLineEdit;
  m_base_check = new QCheckBox(tr("Hex"));
  m_set_value = new QPushButton(tr("Set &Value"));
  m_data_preview = new QLabel;

  m_base_check->setLayoutDirection(Qt::RightToLeft);
  m_data_edit->setPlaceholderText(tr("Value"));
  m_data_preview->setBackgroundRole(QPalette::AlternateBase);
  m_data_preview->setAutoFillBackground(true);

  input_layout->addWidget(m_data_edit);
  input_layout->addWidget(m_base_check);

  auto* target_layout = new QHBoxLayout;
  QLabel* target_label = new QLabel(tr("PC Target:   "), this);
  m_target_address = new QComboBox();
  m_target_address->setDuplicatesEnabled(false);
  m_target_address->setEditable(false);
  m_target_address->setMaxCount(10);
  m_target_address->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  target_layout->addWidget(target_label);
  target_layout->addWidget(m_target_address);

  // Input types
  m_input_combo = new QComboBox;
  m_input_combo->setMaxVisibleItems(20);
  // Order here determines combo list order.
  m_input_combo->addItem(tr("Hex Byte String"), int(Type::HexString));
  m_input_combo->addItem(tr("ASCII"), int(Type::ASCII));
  m_input_combo->addItem(tr("Float"), int(Type::Float32));
  m_input_combo->addItem(tr("Double"), int(Type::Double));
  m_input_combo->addItem(tr("Unsigned 8"), int(Type::Unsigned8));
  m_input_combo->addItem(tr("Unsigned 16"), int(Type::Unsigned16));
  m_input_combo->addItem(tr("Unsigned 32"), int(Type::Unsigned32));
  m_input_combo->addItem(tr("Signed 8"), int(Type::Signed8));
  m_input_combo->addItem(tr("Signed 16"), int(Type::Signed16));
  m_input_combo->addItem(tr("Signed 32"), int(Type::Signed32));

  // Search Options
  auto* search_group = new QGroupBox(tr("Search"));
  auto* search_layout = new QVBoxLayout;
  search_group->setLayout(search_layout);

  m_find_next = new QPushButton(tr("Find &Next"));
  m_find_previous = new QPushButton(tr("Find &Previous"));
  m_result_label = new QLabel;

  search_layout->addWidget(m_find_next);
  search_layout->addWidget(m_find_previous);
  search_layout->addWidget(m_result_label);
  search_layout->setSpacing(1);

  // Address Space
  auto* address_space_group = new QGroupBox(tr("Address Space"));
  auto* address_space_layout = new QVBoxLayout;
  address_space_group->setLayout(address_space_layout);

  // i18n: One of the options shown below "Address Space". "Effective" addresses are the addresses
  // used directly by the CPU and may be subject to translation via the MMU to physical addresses.
  m_address_space_effective = new QRadioButton(tr("Effective"));
  // i18n: One of the options shown below "Address Space". "Auxiliary" is the address space of ARAM
  // (Auxiliary RAM).
  m_address_space_auxiliary = new QRadioButton(tr("Auxiliary"));
  // i18n: One of the options shown below "Address Space". "Physical" is the address space that
  // reflects how devices (e.g. RAM) is physically wired up.
  m_address_space_physical = new QRadioButton(tr("Physical"));

  address_space_layout->addWidget(m_address_space_effective);
  address_space_layout->addWidget(m_address_space_auxiliary);
  address_space_layout->addWidget(m_address_space_physical);
  address_space_layout->setSpacing(1);

  // Data Type
  auto* displaytype_group = new QGroupBox(tr("Display Type"));
  auto* displaytype_layout = new QVBoxLayout;
  displaytype_group->setLayout(displaytype_layout);

  m_display_combo = new QComboBox;
  m_display_combo->setMaxVisibleItems(20);
  m_display_combo->addItem(tr("Hex 8"), int(Type::Hex8));
  m_display_combo->addItem(tr("Hex 16"), int(Type::Hex16));
  m_display_combo->addItem(tr("Hex 32"), int(Type::Hex32));
  m_display_combo->addItem(tr("Unsigned 8"), int(Type::Unsigned8));
  m_display_combo->addItem(tr("Unsigned 16"), int(Type::Unsigned16));
  m_display_combo->addItem(tr("Unsigned 32"), int(Type::Unsigned32));
  m_display_combo->addItem(tr("Signed 8"), int(Type::Signed8));
  m_display_combo->addItem(tr("Signed 16"), int(Type::Signed16));
  m_display_combo->addItem(tr("Signed 32"), int(Type::Signed32));
  m_display_combo->addItem(tr("ASCII"), int(Type::ASCII));
  m_display_combo->addItem(tr("Float"), int(Type::Float32));
  m_display_combo->addItem(tr("Double"), int(Type::Double));

  m_align_combo = new QComboBox;
  m_align_combo->addItem(tr("Fixed Alignment"));
  m_align_combo->addItem(tr("Type-based Alignment"), 0);
  m_align_combo->addItem(tr("No Alignment"), 1);

  m_row_length_combo = new QComboBox;
  m_row_length_combo->addItem(tr("4 Bytes"), 4);
  m_row_length_combo->addItem(tr("8 Bytes"), 8);
  m_row_length_combo->addItem(tr("16 Bytes"), 16);

  m_row_length_combo->setCurrentIndex(2);
  m_dual_check = new QCheckBox(tr("Dual View"));

  displaytype_layout->addWidget(m_display_combo);
  displaytype_layout->addWidget(m_align_combo);
  displaytype_layout->addWidget(m_row_length_combo);
  displaytype_layout->addWidget(m_dual_check);

  // MBP options
  auto* bp_group = new QGroupBox(tr("Memory breakpoint options"));
  auto* bp_layout = new QVBoxLayout;
  bp_group->setLayout(bp_layout);

  // i18n: This string is used for a radio button that represents the type of
  // memory breakpoint that gets triggered when a read operation or write operation occurs.
  // The string is not a command to read and write something or to allow reading and writing.
  m_bp_read_write = new QRadioButton(tr("Read and write"));
  // i18n: This string is used for a radio button that represents the type of
  // memory breakpoint that gets triggered when a read operation occurs.
  // The string does not mean "read-only" in the sense that something cannot be written to.
  m_bp_read_only = new QRadioButton(tr("Read only"));
  // i18n: This string is used for a radio button that represents the type of
  // memory breakpoint that gets triggered when a write operation occurs.
  // The string does not mean "write-only" in the sense that something cannot be read from.
  m_bp_write_only = new QRadioButton(tr("Write only"));
  m_bp_log_check = new QCheckBox(tr("Log"));

  bp_layout->addWidget(m_bp_read_write);
  bp_layout->addWidget(m_bp_read_only);
  bp_layout->addWidget(m_bp_write_only);
  bp_layout->addWidget(m_bp_log_check);
  bp_layout->setSpacing(1);

  // Notes
  m_note_group = new QGroupBox(tr("Notes"));
  auto* note_layout = new QVBoxLayout;
  m_note_list = new QListWidget;
  m_search_notes = new QLineEdit;
  m_search_notes->setPlaceholderText(tr("Filter Note List"));

  m_note_group->setLayout(note_layout);
  note_layout->addWidget(m_note_list);
  note_layout->addWidget(m_search_notes);

  // Sidebar
  auto* sidebar = new QWidget;
  auto* sidebar_layout = new QVBoxLayout;

  // Sidebar top menu
  QMenuBar* menubar = new QMenuBar(sidebar);
  menubar->setNativeMenuBar(false);

  QMenu* menu_view = new QMenu(tr("&View"), menubar);
  auto* show_notes =
      menu_view->addAction(tr("&Show symbols and notes"), this,
                           [this](bool checked) { m_memory_view->ShowSymbols(checked); });
  show_notes->setCheckable(true);
  show_notes->setChecked(true);
  menubar->addMenu(menu_view);

  auto* auto_update_action =
      menu_view->addAction(tr("Auto update memory values"), this,
                           [this](bool checked) { m_auto_update_enabled = checked; });
  auto_update_action->setCheckable(true);
  auto_update_action->setChecked(true);

  auto* highlight_update_action =
      menu_view->addAction(tr("Highlight recently changed values"), this,
                           [this](bool checked) { m_memory_view->ToggleHighlights(checked); });
  highlight_update_action->setCheckable(true);
  highlight_update_action->setChecked(true);

  menu_view->addAction(tr("Highlight color"), this, [this] { m_memory_view->SetHighlightColor(); });

  QMenu* menu_import = new QMenu(tr("&Import"), menubar);
  menu_import->addAction(tr("&Load file to current address"), this,
                         &MemoryWidget::OnSetValueFromFile);
  menubar->addMenu(menu_import);

  QMenu* menu_export = new QMenu(tr("&Export"), menubar);
  menu_export->addAction(tr("Dump &MRAM"), this, &MemoryWidget::OnDumpMRAM);
  menu_export->addAction(tr("Dump &ExRAM"), this, &MemoryWidget::OnDumpExRAM);
  menu_export->addAction(tr("Dump &ARAM"), this, &MemoryWidget::OnDumpARAM);
  menu_export->addAction(tr("Dump &FakeVMEM"), this, &MemoryWidget::OnDumpFakeVMEM);
  menubar->addMenu(menu_export);

  sidebar_layout->setSpacing(1);
  sidebar->setLayout(sidebar_layout);
  sidebar_layout->addItem(new QSpacerItem(1, 20));
  sidebar_layout->addWidget(m_address_splitter);
  sidebar_layout->addLayout(input_layout);
  sidebar_layout->addWidget(m_input_combo);
  sidebar_layout->addItem(new QSpacerItem(1, 10));
  sidebar_layout->addWidget(m_data_preview);
  sidebar_layout->addWidget(m_set_value);
  sidebar_layout->addLayout(target_layout);
  sidebar_layout->addItem(new QSpacerItem(1, 10));
  sidebar_layout->addWidget(search_group);
  sidebar_layout->addItem(new QSpacerItem(1, 10));
  sidebar_layout->addWidget(displaytype_group);
  sidebar_layout->addItem(new QSpacerItem(1, 10));
  sidebar_layout->addWidget(address_space_group);
  sidebar_layout->addItem(new QSpacerItem(1, 10));
  sidebar_layout->addWidget(bp_group);
  sidebar_layout->addWidget(m_note_group);
  sidebar_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

  // Splitter
  m_splitter = new QSplitter(Qt::Horizontal);

  auto* sidebar_scroll = new QScrollArea;
  sidebar_scroll->setWidget(sidebar);
  sidebar_scroll->setWidgetResizable(true);

  m_memory_view = new MemoryViewWidget(this);

  m_splitter->addWidget(m_memory_view);
  m_splitter->addWidget(sidebar_scroll);
  m_splitter->setStretchFactor(0, 3);
  m_splitter->setStretchFactor(1, 1);

  layout->addWidget(m_splitter);

  auto* widget = new QWidget;
  widget->setLayout(layout);
  setWidget(widget);
  UpdateNotes();
}

void MemoryWidget::ConnectWidgets()
{
  connect(m_search_address, &QComboBox::currentTextChanged, this, &MemoryWidget::OnSearchAddress);
  connect(m_search_offset, &QLineEdit::textChanged, this, &MemoryWidget::OnSearchAddress);
  connect(m_data_edit, &QLineEdit::textChanged, this, &MemoryWidget::ValidateAndPreviewInputValue);

  connect(m_input_combo, &QComboBox::currentIndexChanged, this,
          &MemoryWidget::ValidateAndPreviewInputValue);
  connect(m_set_value, &QPushButton::clicked, this, &MemoryWidget::OnSetValue);
  connect(m_target_address, &QComboBox::activated, this, [this] {
    bool ok = false;
    const u32 addr = m_target_address->currentData().toUInt(&ok);
    if (ok)
      m_memory_view->SetAddress(addr);
  });
  connect(m_find_next, &QPushButton::clicked, this, &MemoryWidget::OnFindNextValue);
  connect(m_find_previous, &QPushButton::clicked, this, &MemoryWidget::OnFindPreviousValue);

  for (auto* radio :
       {m_address_space_effective, m_address_space_auxiliary, m_address_space_physical})
  {
    connect(radio, &QRadioButton::toggled, this, &MemoryWidget::OnAddressSpaceChanged);
  }
  for (auto* combo : {m_display_combo, m_align_combo, m_row_length_combo})
  {
    connect(combo, &QComboBox::currentIndexChanged, this, &MemoryWidget::OnDisplayChanged);
  }

  connect(m_dual_check, &QCheckBox::toggled, this, &MemoryWidget::OnDisplayChanged);

  for (auto* radio : {m_bp_read_write, m_bp_read_only, m_bp_write_only})
    connect(radio, &QRadioButton::toggled, this, &MemoryWidget::OnBPTypeChanged);

  connect(m_base_check, &QCheckBox::toggled, this, &MemoryWidget::ValidateAndPreviewInputValue);
  connect(m_bp_log_check, &QCheckBox::toggled, this, &MemoryWidget::OnBPLogChanged);
  connect(m_memory_view, &MemoryViewWidget::BreakpointsChanged, this,
          &MemoryWidget::BreakpointsChanged);

  connect(m_note_list, &QListWidget::itemClicked, this, &MemoryWidget::OnSelectNote);
  connect(m_memory_view, &MemoryViewWidget::NotesChanged, this, &MemoryWidget::UpdateNotes);
  connect(m_search_notes, &QLineEdit::textChanged, this, &MemoryWidget::OnSearchNotes);
  connect(m_memory_view, &MemoryViewWidget::ShowCode, this, &MemoryWidget::ShowCode);
  connect(m_memory_view, &MemoryViewWidget::RequestWatch, this, &MemoryWidget::RequestWatch);
}

void MemoryWidget::closeEvent(QCloseEvent*)
{
  Settings::Instance().SetMemoryVisible(false);
}

void MemoryWidget::showEvent(QShowEvent* event)
{
  RegisterAfterFrameEventCallback();
  Update();
}

void MemoryWidget::hideEvent(QHideEvent* event)
{
  RemoveAfterFrameEventCallback();
}

void MemoryWidget::RegisterAfterFrameEventCallback()
{
  m_VI_end_field_event = VIEndFieldEvent::Register([this] { AutoUpdateTable(); }, "MemoryWidget");
}

void MemoryWidget::RemoveAfterFrameEventCallback()
{
  m_VI_end_field_event.reset();
}

void MemoryWidget::AutoUpdateTable()
{
  if (!isVisible() || !m_auto_update_enabled)
    return;

  m_memory_view->UpdateOnFrameEnd();
}

void MemoryWidget::DisasmUpdate()
{
  if (Core::GetState() != Core::State::Paused)
    return;

  auto& system = Core::System::GetInstance();
  auto& power_pc = system.GetPowerPC();
  auto& ppc_state = system.GetPPCState();
  Core::CPUThreadGuard guard(system);

  const u32 address = ppc_state.pc;
  const std::string inst_str = power_pc.GetDebugInterface().Disassemble(&guard, address);

  if (!IsInstructionLoadStore(inst_str))
  {
    m_target_address->setCurrentIndex(-1);
    return;
  }

  std::optional<u32> target_memory =
      power_pc.GetDebugInterface().GetMemoryAddressFromInstruction(inst_str);

  if (!target_memory.has_value())
  {
    m_target_address->setCurrentText(QStringLiteral("-"));
    return;
  }

  const QString addr_str = QString::number(target_memory.value(), 16);
  const int index = m_target_address->findText(addr_str);
  if (index != -1)
    m_target_address->removeItem(index);

  m_target_address->insertItem(0, addr_str, target_memory.value());
  m_target_address->setCurrentIndex(0);
}

void MemoryWidget::Update()
{
  if (!isVisible())
    return;

  m_memory_view->UpdateDisbatcher(MemoryViewWidget::UpdateType::Addresses);
  update();
}

void MemoryWidget::LoadSettings()
{
  QSettings& settings = Settings::GetQSettings();

  const int combo_index = settings.value(QStringLiteral("memorywidget/inputcombo"), 1).toInt();

  m_input_combo->setCurrentIndex(combo_index);

  const bool address_space_effective =
      settings.value(QStringLiteral("memorywidget/addrspace_effective"), true).toBool();
  const bool address_space_auxiliary =
      settings.value(QStringLiteral("memorywidget/addrspace_auxiliary"), false).toBool();
  const bool address_space_physical =
      settings.value(QStringLiteral("memorywidget/addrspace_physical"), false).toBool();

  m_address_space_effective->setChecked(address_space_effective);
  m_address_space_auxiliary->setChecked(address_space_auxiliary);
  m_address_space_physical->setChecked(address_space_physical);

  const int display_index = settings.value(QStringLiteral("memorywidget/display_type"), 2).toInt();
  const int length_index = settings.value(QStringLiteral("memorywidget/row_length"), 0).toUInt();
  const int align_index = settings.value(QStringLiteral("memorywidget/alignment"), 2).toUInt();
  const bool dual_view_index = settings.value(QStringLiteral("memorywidget/dual_view"), 0).toBool();

  m_display_combo->setCurrentIndex(display_index);
  m_row_length_combo->setCurrentIndex(length_index);
  m_align_combo->setCurrentIndex(align_index);
  m_dual_check->setChecked(dual_view_index);

  bool bp_rw = settings.value(QStringLiteral("memorywidget/bpreadwrite"), true).toBool();
  bool bp_r = settings.value(QStringLiteral("memorywidget/bpread"), false).toBool();
  bool bp_w = settings.value(QStringLiteral("memorywidget/bpwrite"), false).toBool();
  bool bp_log = settings.value(QStringLiteral("memorywidget/bplog"), true).toBool();

  if (bp_rw)
    m_memory_view->SetBPType(MemoryViewWidget::BPType::ReadWrite);
  else if (bp_r)
    m_memory_view->SetBPType(MemoryViewWidget::BPType::ReadOnly);
  else
    m_memory_view->SetBPType(MemoryViewWidget::BPType::WriteOnly);

  m_bp_read_write->setChecked(bp_rw);
  m_bp_read_only->setChecked(bp_r);
  m_bp_write_only->setChecked(bp_w);
  m_bp_log_check->setChecked(bp_log);
}

void MemoryWidget::SaveSettings()
{
  QSettings& settings = Settings::GetQSettings();

  settings.setValue(QStringLiteral("memorywidget/inputcombo"), m_input_combo->currentIndex());

  settings.setValue(QStringLiteral("memorywidget/addrspace_effective"),
                    m_address_space_effective->isChecked());
  settings.setValue(QStringLiteral("memorywidget/addrspace_auxiliary"),
                    m_address_space_auxiliary->isChecked());
  settings.setValue(QStringLiteral("memorywidget/addrspace_physical"),
                    m_address_space_physical->isChecked());

  settings.setValue(QStringLiteral("memorywidget/display_type"), m_display_combo->currentIndex());
  settings.setValue(QStringLiteral("memorywidget/row_length"), m_row_length_combo->currentIndex());
  settings.setValue(QStringLiteral("memorywidget/alignment"), m_align_combo->currentIndex());
  settings.setValue(QStringLiteral("memorywidget/dual_view"), m_dual_check->isChecked());

  settings.setValue(QStringLiteral("memorywidget/bpreadwrite"), m_bp_read_write->isChecked());
  settings.setValue(QStringLiteral("memorywidget/bpread"), m_bp_read_only->isChecked());
  settings.setValue(QStringLiteral("memorywidget/bpwrite"), m_bp_write_only->isChecked());
  settings.setValue(QStringLiteral("memorywidget/bplog"), m_bp_log_check->isChecked());
}

void MemoryWidget::OnAddressSpaceChanged()
{
  AddressSpace::Type space;

  if (m_address_space_effective->isChecked())
    space = AddressSpace::Type::Effective;
  else if (m_address_space_auxiliary->isChecked())
    space = AddressSpace::Type::Auxiliary;
  else
    space = AddressSpace::Type::Physical;

  m_memory_view->SetAddressSpace(space);

  SaveSettings();
}

void MemoryWidget::OnDisplayChanged()
{
  const auto type = static_cast<Type>(m_display_combo->currentData().toInt());
  int bytes_per_row = m_row_length_combo->currentData().toInt();
  int alignment;
  bool dual_view = m_dual_check->isChecked();

  if (type == Type::Double && bytes_per_row == 4)
    bytes_per_row = 8;

  // Alignment: First (fixed) option equals bytes per row. 'currentData' is correct for other
  // options. Type-based must be calculated in memoryviewwidget and is left at 0. "No alignment" is
  // equivalent to a value of 1.
  if (m_align_combo->currentIndex() == 0)
    alignment = bytes_per_row;
  else
    alignment = m_align_combo->currentData().toInt();

  m_memory_view->SetDisplay(type, bytes_per_row, alignment, dual_view);

  SaveSettings();
}

void MemoryWidget::OnBPLogChanged()
{
  m_memory_view->SetBPLoggingEnabled(m_bp_log_check->isChecked());
  SaveSettings();
}

void MemoryWidget::OnBPTypeChanged()
{
  bool read_write = m_bp_read_write->isChecked();
  bool read_only = m_bp_read_only->isChecked();

  MemoryViewWidget::BPType type;

  if (read_write)
    type = MemoryViewWidget::BPType::ReadWrite;
  else if (read_only)
    type = MemoryViewWidget::BPType::ReadOnly;
  else
    type = MemoryViewWidget::BPType::WriteOnly;

  m_memory_view->SetBPType(type);

  SaveSettings();
}

void MemoryWidget::SetAddress(u32 address)
{
  // Store current and target address in combo box
  const QSignalBlocker blocker(m_search_address);
  const QString current_text = m_search_address->currentText();
  const QString target_addr = QString::number(address, 16);
  bool good;
  const u32 current_addr = current_text.toUInt(&good, 16);

  if (good)
  {
    AddressSpace::Accessors* accessors =
        AddressSpace::GetAccessors(m_memory_view->GetAddressSpace());

    Core::CPUThreadGuard guard(Core::System::GetInstance());
    good = accessors->IsValidAddress(guard, current_addr);
  }

  if (m_search_address->findText(current_text) == -1 && good)
    m_search_address->insertItem(0, current_text);

  if (m_search_address->findText(target_addr) == -1)
    m_search_address->insertItem(0, target_addr);

  m_search_address->setCurrentText(target_addr);

  m_memory_view->SetAddress(address);
  Settings::Instance().SetMemoryVisible(true);
  raise();

  m_memory_view->setFocus();
}

void MemoryWidget::OnSearchAddress()
{
  const auto target_addr = GetTargetAddress();

  if (target_addr.is_good_address && target_addr.is_good_offset)
    m_memory_view->SetAddress(target_addr.address);

  QFont addr_font, offset_font;
  QPalette addr_palette, offset_palette;

  if (!target_addr.is_good_address)
  {
    addr_font.setBold(true);
    addr_palette.setColor(QPalette::Text, Qt::red);
  }

  if (!target_addr.is_good_offset)
  {
    offset_font.setBold(true);
    offset_palette.setColor(QPalette::Text, Qt::red);
  }

  m_search_address->setFont(addr_font);
  m_search_address->setPalette(addr_palette);
  m_search_offset->setFont(offset_font);
  m_search_offset->setPalette(offset_palette);
}

void MemoryWidget::ValidateAndPreviewInputValue()
{
  m_data_preview->clear();
  QString input_text = m_data_edit->text();
  const auto input_type = static_cast<Type>(m_input_combo->currentData().toInt());

  m_base_check->setEnabled(input_type == Type::Unsigned32 || input_type == Type::Signed32 ||
                           input_type == Type::Unsigned16 || input_type == Type::Signed16 ||
                           input_type == Type::Unsigned8 || input_type == Type::Signed8);

  if (input_text.isEmpty())
    return;

  // Remove any spaces
  if (input_type != Type::ASCII)
    input_text.remove(QLatin1Char(' '));

  if (m_base_check->isChecked())
  {
    if (input_text.startsWith(QLatin1Char('-')))
      input_text.insert(1, QStringLiteral("0x"));
    else
      input_text.prepend(QStringLiteral("0x"));
  }

  QFont font;
  QPalette palette;
  std::vector<u8> bytes = m_memory_view->ConvertTextToBytes(input_type, input_text);

  if (!bytes.empty())
  {
    QString hex_string;
    std::string s;

    for (const u8 c : bytes)
      s.append(fmt::format("{:02x}", c));

    hex_string = QString::fromStdString(s);
    int output_length = hex_string.length();

    if (output_length > 16)
    {
      hex_string.truncate(16);
      output_length = hex_string.length();
      hex_string.append(QStringLiteral("..."));
    }

    for (int i = 2; i < output_length; i += 2)
      hex_string.insert(output_length - i, QLatin1Char{' '});

    m_data_preview->setText(hex_string);
  }
  else
  {
    font.setBold(true);
    palette.setColor(QPalette::Text, Qt::red);
  }

  m_data_edit->setFont(font);
  m_data_edit->setPalette(palette);
}

QByteArray MemoryWidget::GetInputData() const
{
  // Empty or invalid input data returns an empty array.
  if (m_data_preview->text().isEmpty())
    return QByteArray();

  const auto input_type = static_cast<Type>(m_input_combo->currentData().toInt());

  // Ascii might be truncated, pull from data edit box.
  if (input_type == Type::ASCII)
    return QByteArray(m_data_edit->text().toUtf8());

  // If we are doing a large array of hex bytes
  if (input_type == Type::HexString)
    return QByteArray::fromHex(m_data_edit->text().toUtf8());

  // Data preview has exactly what we want to input, so pull it from there.
  return QByteArray::fromHex(m_data_preview->text().toUtf8());
}

void MemoryWidget::OnSetValue()
{
  if (!Core::IsRunning())
    return;

  auto target_addr = GetTargetAddress();

  if (!target_addr.is_good_address)
  {
    ModalMessageBox::critical(this, tr("Error"), tr("Bad address provided."));
    return;
  }

  if (!target_addr.is_good_offset)
  {
    ModalMessageBox::critical(this, tr("Error"), tr("Bad offset provided."));
    return;
  }

  const QByteArray bytes = GetInputData();

  // Invalid input will give an empty array.
  if (bytes.isEmpty())
  {
    ModalMessageBox::critical(this, tr("Error"), tr("Bad value provided."));
    return;
  }

  Core::CPUThreadGuard guard(Core::System::GetInstance());

  AddressSpace::Accessors* accessors = AddressSpace::GetAccessors(m_memory_view->GetAddressSpace());
  u32 end_address = target_addr.address + static_cast<u32>(bytes.size()) - 1;

  if (!accessors->IsValidAddress(guard, target_addr.address) ||
      !accessors->IsValidAddress(guard, end_address))
  {
    ModalMessageBox::critical(this, tr("Error"), tr("Target address range is invalid."));
    return;
  }

  for (const char c : bytes)
    accessors->WriteU8(guard, target_addr.address++, static_cast<u8>(c));

  Update();
}

void MemoryWidget::OnSetValueFromFile()
{
  if (!Core::IsRunning())
    return;

  auto target_addr = GetTargetAddress();

  if (!target_addr.is_good_address)
  {
    ModalMessageBox::critical(this, tr("Error"), tr("Bad address provided."));
    return;
  }

  if (!target_addr.is_good_offset)
  {
    ModalMessageBox::critical(this, tr("Error"), tr("Bad offset provided."));
    return;
  }

  QString path = QFileDialog::getOpenFileName(this, tr("Select a file"), QDir::currentPath(),
                                              tr("All files (*)"));
  if (path.isNull())
  {
    return;
  }

  File::IOFile f(path.toStdString(), "rb");

  if (!f)
  {
    ModalMessageBox::critical(this, tr("Error"), tr("Unable to open file."));
    return;
  }

  const u64 file_length = f.GetSize();
  std::vector<u8> file_contents(file_length);
  if (!f.ReadBytes(file_contents.data(), file_length))
  {
    ModalMessageBox::critical(this, tr("Error"), tr("Unable to read file."));
    return;
  }

  AddressSpace::Accessors* accessors = AddressSpace::GetAccessors(m_memory_view->GetAddressSpace());

  Core::CPUThreadGuard guard(Core::System::GetInstance());

  for (u8 b : file_contents)
    accessors->WriteU8(guard, target_addr.address++, b);

  Update();
}

void MemoryWidget::OnSearchNotes()
{
  m_note_filter = m_search_notes->text();
  UpdateNotes();
}

void MemoryWidget::OnSelectNote()
{
  const auto items = m_note_list->selectedItems();
  if (items.isEmpty())
    return;

  const u32 address = items[0]->data(Qt::UserRole).toUInt();

  SetAddress(address);
}

void MemoryWidget::UpdateNotes()
{
  if (g_symbolDB.Notes().empty())
  {
    m_note_group->hide();
    return;
  }

  m_note_group->show();

  QString selection = m_note_list->selectedItems().isEmpty() ?
                          QStringLiteral("") :
                          m_note_list->selectedItems()[0]->text();
  m_note_list->clear();

  for (const auto& note : g_symbolDB.Notes())
  {
    QString name = QString::fromStdString(note.second.name);

    auto* item = new QListWidgetItem(name);
    if (name == selection)
      item->setSelected(true);

    item->setData(Qt::UserRole, note.second.address);

    if (name.toUpper().indexOf(m_note_filter.toUpper()) != -1)
      m_note_list->addItem(item);
  }

  m_note_list->sortItems();
}

static void DumpArray(const std::string& filename, const u8* data, size_t length)
{
  if (!data)
    return;

  File::IOFile f(filename, "wb");

  if (!f)
  {
    ModalMessageBox::critical(
        nullptr, QObject::tr("Error"),
        QObject::tr("Failed to dump %1: Can't open file").arg(QString::fromStdString(filename)));
    return;
  }

  if (!f.WriteBytes(data, length))
  {
    ModalMessageBox::critical(nullptr, QObject::tr("Error"),
                              QObject::tr("Failed to dump %1: Failed to write to file")
                                  .arg(QString::fromStdString(filename)));
  }
}

void MemoryWidget::OnDumpMRAM()
{
  AddressSpace::Accessors* accessors = AddressSpace::GetAccessors(AddressSpace::Type::Mem1);
  DumpArray(File::GetUserPath(F_MEM1DUMP_IDX), accessors->begin(),
            std::distance(accessors->begin(), accessors->end()));
}

void MemoryWidget::OnDumpExRAM()
{
  AddressSpace::Accessors* accessors = AddressSpace::GetAccessors(AddressSpace::Type::Mem2);
  DumpArray(File::GetUserPath(F_MEM2DUMP_IDX), accessors->begin(),
            std::distance(accessors->begin(), accessors->end()));
}

void MemoryWidget::OnDumpARAM()
{
  AddressSpace::Accessors* accessors = AddressSpace::GetAccessors(AddressSpace::Type::Auxiliary);
  DumpArray(File::GetUserPath(F_ARAMDUMP_IDX), accessors->begin(),
            std::distance(accessors->begin(), accessors->end()));
}

void MemoryWidget::OnDumpFakeVMEM()
{
  AddressSpace::Accessors* accessors = AddressSpace::GetAccessors(AddressSpace::Type::Fake);
  DumpArray(File::GetUserPath(F_FAKEVMEMDUMP_IDX), accessors->begin(),
            std::distance(accessors->begin(), accessors->end()));
}

MemoryWidget::TargetAddress MemoryWidget::GetTargetAddress() const
{
  TargetAddress target;

  // Returns 0 if conversion fails
  const u32 addr = m_search_address->currentText().toUInt(&target.is_good_address, 16);
  target.is_good_address |= m_search_address->currentText().isEmpty();
  const s32 offset = m_search_offset->text().toInt(&target.is_good_offset, 16);
  const u32 neg_offset = offset != std::numeric_limits<s32>::min() ?
                             -offset :
                             u32(std::numeric_limits<s32>::max()) + 1;
  target.is_good_offset |= m_search_offset->text().isEmpty();
  target.is_good_offset &= offset >= 0 || neg_offset <= addr;
  target.is_good_offset &= offset <= 0 || (std::numeric_limits<u32>::max() - u32(offset)) >= addr;

  if (!target.is_good_address || !target.is_good_offset)
    return target;

  if (offset < 0)
    target.address = addr - neg_offset;
  else
    target.address = addr + u32(offset);
  return target;
}

void MemoryWidget::FindValue(bool next)
{
  auto target_addr = GetTargetAddress();

  if (!target_addr.is_good_address)
  {
    m_result_label->setText(tr("Bad address provided."));
    return;
  }

  if (!target_addr.is_good_offset)
  {
    m_result_label->setText(tr("Bad offset provided."));
    return;
  }

  const QByteArray search_for = GetInputData();

  if (search_for.isEmpty())
  {
    m_result_label->setText(tr("Bad Value Given"));
    return;
  }

  if (!m_search_address->currentText().isEmpty())
  {
    // skip the quoted address so we don't potentially refind the last result
    target_addr.address += next ? 1 : -1;
  }

  const std::optional<u32> found_addr = [&] {
    AddressSpace::Accessors* accessors =
        AddressSpace::GetAccessors(m_memory_view->GetAddressSpace());

    Core::CPUThreadGuard guard(Core::System::GetInstance());
    return accessors->Search(guard, target_addr.address,
                             reinterpret_cast<const u8*>(search_for.data()),
                             static_cast<u32>(search_for.size()), next);
  }();

  if (found_addr.has_value())
  {
    m_result_label->setText(tr("Match Found"));

    u32 offset = *found_addr;

    m_search_address->setCurrentText(QStringLiteral("%1").arg(offset, 8, 16, QLatin1Char('0')));
    m_search_offset->clear();

    m_memory_view->SetAddress(offset);

    return;
  }

  m_result_label->setText(tr("No Match"));
}

void MemoryWidget::OnFindNextValue()
{
  FindValue(true);
}

void MemoryWidget::OnFindPreviousValue()
{
  FindValue(false);
}
