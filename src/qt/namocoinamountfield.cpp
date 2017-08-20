#include "namocoinamountfield.h"
#include "qvaluecombobox.h"
#include "namocoinunits.h"

#include "guiconstants.h"

#include <QLabel>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QApplication>
#include <qmath.h>

namocoinAmountField::namocoinAmountField(QWidget *parent):
        QWidget(parent), amount(0), currentUnit(-1)
{
    amount = new QDoubleSpinBox(this);
    amount->setLocale(QLocale::c());
    amount->setDecimals(8);
    amount->installEventFilter(this);
    amount->setMaximumWidth(170);
    amount->setSingleStep(0.001);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(amount);
    unit = new QValueComboBox(this);
    unit->setModel(new namocoinUnits(this));
    layout->addWidget(unit);
    layout->addStretch(1);
    layout->setContentsMargins(0,0,0,0);

    setLayout(layout);
    amount->setFrame(false);
    unit->setFrame(false);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(amount);

    // If one if the widgets changes, the combined content changes as well
    connect(amount, SIGNAL(valueChanged(QString)), this, SIGNAL(textChanged()));
    connect(unit, SIGNAL(currentIndexChanged(int)), this, SLOT(unitChanged(int)));

    // Set default based on configuration
    unitChanged(unit->currentIndex());
}

void namocoinAmountField::setText(const QString &text)
{
    if (text.isEmpty())
        amount->clear();
    else
        amount->setValue(text.toDouble());
}

void namocoinAmountField::clear()
{
    amount->clear();
    unit->setCurrentIndex(0);
}

bool namocoinAmountField::validate()
{
    bool valid = true;
    if (amount->value() == 0.0)
        valid = false;
    if (valid && !namocoinUnits::parse(currentUnit, text(), 0))
        valid = false;

    setValid(valid);

    return valid;
}

void namocoinAmountField::setValid(bool valid)
{
    if (valid)
        amount->setStyleSheet("");
    else
        amount->setStyleSheet(STYLE_INVALID);
}

QString namocoinAmountField::text() const
{
    if (amount->text().isEmpty())
        return QString();
    else
        return amount->text();
}

bool namocoinAmountField::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn)
    {
        // Clear invalid flag on focus
        setValid(true);
    }
    else if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Comma)
        {
            // Translate a comma into a period
            QKeyEvent periodKeyEvent(event->type(), Qt::Key_Period, keyEvent->modifiers(), ".", keyEvent->isAutoRepeat(), keyEvent->count());
            qApp->sendEvent(object, &periodKeyEvent);
            return true;
        }
    }
    return QWidget::eventFilter(object, event);
}

QWidget *namocoinAmountField::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, amount);
    return amount;
}

qint64 namocoinAmountField::value(bool *valid_out) const
{
    qint64 val_out = 0;
    bool valid = namocoinUnits::parse(currentUnit, text(), &val_out);
    if(valid_out)
    {
        *valid_out = valid;
    }
    return val_out;
}

void namocoinAmountField::setValue(qint64 value)
{
    setText(namocoinUnits::format(currentUnit, value));
}

void namocoinAmountField::unitChanged(int idx)
{
    // Use description tooltip for current unit for the combobox
    unit->setToolTip(unit->itemData(idx, Qt::ToolTipRole).toString());

    // Determine new unit ID
    int newUnit = unit->itemData(idx, namocoinUnits::UnitRole).toInt();

    // Parse current value and convert to new unit
    bool valid = false;
    qint64 currentValue = value(&valid);

    currentUnit = newUnit;

    // Set max length after retrieving the value, to prevent truncation
    amount->setDecimals(namocoinUnits::decimals(currentUnit));
    amount->setMaximum(qPow(10, namocoinUnits::amountDigits(currentUnit)) - qPow(10, -amount->decimals()));

    if(valid)
    {
        // If value was valid, re-place it in the widget with the new unit
        setValue(currentValue);
    }
    else
    {
        // If current value is invalid, just clear field
        setText("");
    }
    setValid(true);
}

void namocoinAmountField::setDisplayUnit(int newUnit)
{
    unit->setValue(newUnit);
}
