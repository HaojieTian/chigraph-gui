#include "paramlistwidget.hpp"

#include <KComboBox>
#include <KMessageBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <chi/Context.hpp>
#include <chi/DataType.hpp>
#include <chi/GraphFunction.hpp>
#include <chi/GraphModule.hpp>

#include "execparamlistwidget.hpp"
#include "functionview.hpp"
#include "typeselector.hpp"

QStringList createTypeOptions(const chi::GraphModule& mod) {
	QStringList ret;

	// add the module
	for (const auto& ty : mod.typeNames()) {
		ret << QString::fromStdString(mod.fullName() + ":" + ty);
	}

	// and its dependencies
	for (auto dep : mod.dependencies()) {
		auto depMod = mod.context().moduleByFullName(dep);
		for (const auto& type : depMod->typeNames()) {
			ret << QString::fromStdString(depMod->fullName() + ":" + type);
		}
	}
	return ret;
}

ParamListWidget::ParamListWidget(QWidget* parent) : QWidget(parent) {}

void ParamListWidget::setFunction(FunctionView* func, Type ty) {
	mFunc = func;
	mType = ty;

	if (layout()) { deleteLayout(layout()); }

	auto layout = new QGridLayout;
	setLayout(layout);

	// populate it
	auto& typeVec =
	    ty == Input ? mFunc->function()->dataInputs() : mFunc->function()->dataOutputs();

	auto id = 0;
	for (const auto& param : typeVec) {
		auto edit = new QLineEdit;
		edit->setText(QString::fromStdString(param.name));
		connect(edit, &QLineEdit::textChanged, this, [this, id](const QString& newText) {
			if (mType == Input) {
				mFunc->function()->renameDataInput(id, newText.toStdString());
				refreshEntry();
			} else {
				mFunc->function()->renameDataOutput(id, newText.toStdString());
				refreshExits();
			}
			dirtied();
		});
		layout->addWidget(edit, id, 0, Qt::AlignTop);

		auto tySelector = new TypeSelector(mFunc->function()->module());
		tySelector->setCurrentType(param.type);
		connect(tySelector, &TypeSelector::typeSelected, this,
		        [this, id](const chi::DataType& newType) {
			        if (!newType.valid()) { return; }

			        if (mType == Input) {
				        mFunc->function()->retypeDataInput(id, newType);
				        refreshEntry();
			        } else {
				        mFunc->function()->retypeDataOutput(id, newType);
				        refreshExits();
			        }
			        dirtied();
		        });
		layout->addWidget(tySelector, id, 1, Qt::AlignTop);

		auto deleteButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), {});
		connect(deleteButton, &QAbstractButton::clicked, this, [this, id](bool) {
			if (mType == Input) {
				mFunc->function()->removeDataInput(id);
				refreshEntry();
				setFunction(mFunc, mType);
			} else {
				mFunc->function()->removeDataOutput(id);
				refreshExits();
				setFunction(mFunc, mType);
			}
			dirtied();
		});
		layout->addWidget(deleteButton, id, 2, Qt::AlignTop);

		++id;
	}

	// create the "new" button
	auto newButton = new QPushButton(QIcon::fromTheme("list-add"), {});
	newButton->setSizePolicy({QSizePolicy::Maximum, QSizePolicy::Maximum});
	connect(newButton, &QAbstractButton::clicked, this, [this](bool) {
		if (mType == Input) {
			mFunc->function()->addDataInput(
			    mFunc->function()->context().moduleByFullName("lang")->typeFromName("i32"), "");
			refreshEntry();

			setFunction(mFunc, mType);  // TODO: not the most efficient way...
		} else {
			mFunc->function()->addDataOutput(
			    mFunc->function()->context().moduleByFullName("lang")->typeFromName("i32"), "");
			refreshExits();

			setFunction(mFunc, mType);
		}
		dirtied();
	});
	layout->addWidget(newButton, id, 2, Qt::AlignRight | Qt::AlignTop);
}

void ParamListWidget::refreshEntry() {
	auto entry = mFunc->function()->entryNode();
	if (entry == nullptr) { return; }
	mFunc->refreshGuiForNode(*entry);
}
void ParamListWidget::refreshExits() {
	for (const auto& exit : mFunc->function()->nodesWithType("lang", "exit")) {
		mFunc->refreshGuiForNode(*exit);
	}
}
