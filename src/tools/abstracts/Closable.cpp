#include "Closable.h"

namespace tools::abstracts {

    Closable::~Closable() {
        if (!isClosing()) close();
    }

    void Closable::close() {
        closing = true;
    }

    bool Closable::isClosing() const {
        return closing;
    }

}