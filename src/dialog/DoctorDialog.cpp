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
#include "manager/DataManager.h"

namespace AITalk
{

namespace {
enum Column { ColMember, ColCli, ColStatus, ColVersion, ColPath, ColCount };
}

DoctorDialog::DoctorDialog(AgentHub* hub, QWidget* parent)
    : QDialog(parent)
    , hub(hub)
{
    setWindowTitle(Text("doctor.title"));
    resize(760, 440);

    auto* lay = new QVBoxLayout(this);
    this->summary = new QLabel(this);
    this->summary->setObjectName(QStringLiteral("dialogHint"));
    lay->addWidget(this->summary);

    this->table = new QTableWidget(int(Members::all().size()), ColCount, this);
    this->table->setHorizontalHeaderLabels({ Text("doctor.colMember"), QStringLiteral("CLI"), Text("doctor.colStatus"), Text("doctor.colVersion"), Text("doctor.colPath") });
    this->table->verticalHeader()->hide();
    this->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->table->setSelectionMode(QAbstractItemView::NoSelection);
    this->table->horizontalHeader()->setSectionResizeMode(ColPath, QHeaderView::Stretch);
    lay->addWidget(this->table, 1);

    lay->addWidget(new QLabel(Text("doctor.installHeader"), this));
    this->install = new QPlainTextEdit(this);
    this->install->setObjectName(QStringLiteral("installHint"));
    this->install->setReadOnly(true);
    this->install->setMaximumHeight(110);
    lay->addWidget(this->install);

    auto* buttons = new QDialogButtonBox(this);
    auto* again = buttons->addButton(Text("doctor.recheck"), QDialogButtonBox::ActionRole);
    auto* ok = buttons->addButton(Text("common.ok"), QDialogButtonBox::AcceptRole);
    ok->setProperty("variant", "primary");
    connect(again, &QPushButton::clicked, this, [this] {
        this->hub->refreshBins();
        runChecks();
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    lay->addWidget(buttons);

    runChecks();
}

void DoctorDialog::runChecks()
{
    this->pending = 0;
    this->found = 0;
    QStringList install;
    int row = 0;
    for (const MemberInfo& m : Members::all()) {
        const QString bin = this->hub->binOf(m.id);
        auto set = [this, row](int col, const QString& text) {
            this->table->setItem(row, col, new QTableWidgetItem(text));
        };
        set(ColMember, m.name);
        set(ColCli, m.cli);
        set(ColPath, bin.isEmpty() ? QStringLiteral("-") : bin);
        set(ColVersion, QString());
        if (bin.isEmpty()) {
            set(ColStatus, Text("doctor.statusMissing"));
            install << QStringLiteral("# %1\n%2").arg(m.name, CliLocator::installHint(m.cli));
        } else {
            ++this->found;
            ++this->pending;
            set(ColStatus, Text("doctor.statusChecking"));
            auto* proc = new CliProcess(this);
            connect(proc, &CliProcess::finished, this, [this, proc, row](const CliResult& r) {
                static const QRegularExpression ver(QStringLiteral(R"(\d+\.\d+(\.\d+)?)"));
                const QString out = QString::fromUtf8(r.out + r.err).trimmed();
                const QString v = ver.match(out).captured(0);
                this->table->setItem(row, ColVersion, new QTableWidgetItem(v.isEmpty() ? out.section(QLatin1Char('\n'), 0, 0).left(40) : v));
                this->table->setItem(row, ColStatus, new QTableWidgetItem(r.exitCode == 0 ? Text("doctor.statusOk") : Text("doctor.statusFailed")));
                proc->deleteLater();
                finishOne();
            });
            proc->start(bin, { QStringLiteral("--version") }, QByteArray(), QString(), 20000);
        }
        ++row;
    }
    this->table->resizeColumnsToContents();
    this->table->horizontalHeader()->setSectionResizeMode(ColPath, QHeaderView::Stretch);
    this->install->setPlainText(install.isEmpty() ? Text("doctor.allFound")
                                              : install.join(QStringLiteral("\n\n")));
    this->summary->setText(Text("doctor.summary")
                           .arg(this->found)
                           .arg(Members::all().size()));
}

void DoctorDialog::finishOne()
{
    if (--this->pending <= 0)
        this->table->resizeColumnToContents(ColVersion);
}

} // namespace AITalk
