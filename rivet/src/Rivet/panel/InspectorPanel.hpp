#pragma once

#include <Rivet/panel/EditorPanel.hpp>

namespace Rivet
{
    class InspectorPanel : public EditorPanel
    {
        public:
            InspectorPanel();
        protected:
            void on_draw() override;
    };
}