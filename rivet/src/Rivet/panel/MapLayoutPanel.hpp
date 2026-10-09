#pragma once

#include <Rivet/panel/EditorPanel.hpp>

namespace Rivet
{
    class MapLayoutPanel : public EditorPanel
    {
        public:
            MapLayoutPanel();
        protected:
            void on_draw() override;
    };
}