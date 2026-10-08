#pragma once

#include <Rivet/panel/EditorPanel.hpp>

namespace Rivet
{
    class EntitiesPanel : public EditorPanel
    {
        public:
            EntitiesPanel();
        protected:
            void on_draw() override;
    };
}