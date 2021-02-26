#pragma once

#include <memory>

#include "model.h"
#include "mathUtils.h"

struct ModelInstance {
    static ModelInstance fromModelPtr(std::weak_ptr<Model const> const & nModel);

    std::weak_ptr<Model const> model;
    Mat4 orientation;

    void draw() const;
};
