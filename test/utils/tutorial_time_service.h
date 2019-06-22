//
// Created by Giulio Luzzati on 19/07/18.
//

#ifndef NEWCORE_EXAMPLE_SERVICE_H
#define NEWCORE_EXAMPLE_SERVICE_H

#include "messenger/messenger.h"

struct TimeServiceContext : public MessengerContext{
    int count;
};

class TimeService : public Messenger{
public:
    TimeService(Configuration p):Messenger(p) {}
    Status init() override;
    Status finalize() override;
};

#endif //NEWCORE_EXAMPLE_SERVICE_H
