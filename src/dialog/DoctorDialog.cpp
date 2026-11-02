#include "dialog/DoctorDialog.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QVBoxLayout>

#include "agent/AgentHub.h"
#include "agent/CliLocator.h"
#include "agent/CliProcess.h"
#include "core/Member.h"
#include "util/DataManager.h"

namespace {
enum Column { ColMember, ColCli, ColStatus, ColVersion, ColPath, ColCount };
}

DoctorDialog::DoctorDialog(AgentHub* hub, QWidget* parent)
    : QDialog(parent)
    , m_hub(hub)
{
    setWindowTitle(Text("doctor.title"));
    resize(760, 440);

    auto* lay = new QVBoxLayout(this);
    m_summary = new QLabel(this);
    m_summary->setObjectName(QStringLiteral("dialogHint"));
    lay->addWidget(m_summary);

    m_table = new QTableWidget(int(Members::all().size()), ColCount, this);
    m_table->setHorizontalHeaderLabels({ Text("doctor.colMember"), QStringLiteral("CLI"), Text("doctor.colStatus"), Text("doctor.colVersion"), Text("doctor.colPath") });
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->horizontalHeader()->setSectionResizeMode(ColPath, QHeaderView::Stretch);
    lay->addWidget(m_table, 1);

    lay->addWidget(new QLabel(Text("doctor.installHeader"), this));
    m_install = new QPlainTextEdit(this);
    m_install->setObjectName(QStringLiteral("installHint"));
    m_install->setReadOnly(true);
    m_install->setMaximumHeight(110);
    lay->addWidget(m_install);

    auto* buttons = new QDialogButtonBox(this);
    auto* again = buttons->addButton(Text("doctor.recheck"), QDialogButtonBox::ActionRole);
    auto* ok = buttons->addButton(Text("common.ok"), QDialogButtonBox::AcceptRole);
    ok->setProperty("variant", "primary");
    connect(again, &QPushButton::clicked, this, [this] {
        m_hub->refreshBins();
        runChecks();
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    lay->addWidget(buttons);

    runChecks();
}

void DoctorDialog::runChecks()
{
    m_pending = 0;
    m_found = 0;
    QStringList install;
    int row = 0;
    for (const MemberInfo& m : Members::all()) {
        const QString bin = m_hub->binOf(m.id);
        auto set = [this, row](int col, const QString& text) {
            m_table->setItem(row, col, new QTableWidgetItem(text));
        };
        set(ColMember, m.name);
        set(ColCli, m.cli);
        set(ColPath, bin.isEmpty() ? QStringLiteral("-") : bin);
        set(ColVersion, QString());
        if (bin.isEmpty()) {
            set(ColStatus, Text("doctor.statusMissing"));
            install << QStringLiteral("# %1\n%2").arg(m.name, CliLocator::installHint(m.cli));
        } else {
            ++m_found;
            ++m_pending;
            set(ColStatus, Text("doctor.statusChecking"));
            auto* proc = new CliProcess(this);
            connect(proc, &CliProcess::finished, this, [this, proc, row](const CliResult& r) {
                static const QRegularExpression ver(QStringLiteral(R"(\d+\.\d+(\.\d+)?)"));
                const QString out = QString::fromUtf8(r.out + r.err).trimmed();
                const QString v = ver.match(out).captured(0);
                m_table->setItem(row, ColVersion, new QTableWidgetItem(v.isEmpty() ? out.section(QLatin1Char('\n'), 0, 0).left(40) : v));
                m_table->setItem(row, ColStatus, new QTableWidgetItem(r.exitCode == 0 ? Text("doctor.statusOk") : Text("doctor.statusFailed")));
                proc->deleteLater();
                finishOne();
            });
            proc->start(bin, { QStringLiteral("--version") }, QByteArray(), QString(), 20000);
        }
        ++row;
    }
    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setSectionResizeMode(ColPath, QHeaderView::Stretch);
    m_install->setPlainText(install.isEmpty() ? Text("doctor.allFound")
                                              : install.join(QStringLiteral("\n\n")));
    m_summary->setText(Text("doctor.summary")
                           .arg(m_found)
                           .arg(Members::all().size()));
}

void DoctorDialog::finishOne()
{
    if (--m_pending <= 0)
        m_table->resizeColumnToContents(ColVersion);
}
