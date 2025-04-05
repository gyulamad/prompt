#pragma once

namespace tools::agency::agents {

    template<typename T>
    class JSONSerializableAgent: public Agent<T>, public JSONSerializable {
    public:
        using Agent<T>::Agent;

        virtual ~JSONSerializableAgent() {}

        virtual JSON toJSON() const UNIMP_THROW

        virtual void fromJSON(const JSON& j) UNIMP_THROW
    };

}