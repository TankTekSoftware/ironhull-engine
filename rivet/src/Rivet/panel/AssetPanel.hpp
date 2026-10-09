#pragma once

#include <Rivet/panel/EditorPanel.hpp>

namespace Rivet
{
    class AssetPanel : public EditorPanel
    {
        public:
            AssetPanel();
        protected:
            void on_draw() override;
    };
}