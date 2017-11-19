#include "connectdialog.h"
#include "ui_connectdialog.h"

ConnectDialog::ConnectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectDialog)
{
    ui->setupUi(this);
}

ConnectDialog::~ConnectDialog()
{
    delete ui;
}

QString ConnectDialog::host()
{
    return ui->lineEditHost->text();
}

QString ConnectDialog::port()
{
    return ui->lineEditPort->text();
}

QString ConnectDialog::user()
{
    return ui->lineEditUser->text();
}

QString ConnectDialog::nick()
{
    return ui->lineEditNick->text();
}
