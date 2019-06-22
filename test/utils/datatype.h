//
// Created by giulio on 16/09/18.
//

#ifndef QHM_DATATYPE_H
#define QHM_DATATYPE_H

inline bool json_has(const nlohmann::json j, std::string field) {
    try {
        j.at(field);
    } catch (std::exception e) {
        return false;
    }
    return true;
}

class AbstractDatatype {
public:
    nlohmann::json  backing_json_obj;
    bool            has(const std::string& field) { return json_has(backing_json_obj, field); }
};

static std::vector<std::shared_ptr<AbstractDatatype>> json_array_to_native(const nlohmann::json& array){
    std::vector<std::shared_ptr<AbstractDatatype>> ret;
    for(auto el: array) {
        auto addendum = std::make_shared<AbstractDatatype>(AbstractDatatype());
        addendum->backing_json_obj = el;
        ret.push_back(addendum);
    }
    return ret;
}

template <typename T>
static std::vector<std::shared_ptr<T>> convert_generic_array(const std::vector<std::shared_ptr<AbstractDatatype>> & array){
    std::vector<std::shared_ptr<T>> ret;
    for(auto el: array){
        ret.push_back(std::make_shared<T>(*(T*)el.get()));
    }
    return ret;
}
#endif //QHM_DATATYPE_H
