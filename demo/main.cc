#include <QApplication>
#include "demo_widget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    DemoWidget widget;
    widget.show();
    
    return app.exec();
}