#pragma once

#include <Rivet/panel/EditorPanel.hpp>

namespace Rivet
{
    class ConsolePanel : public EditorPanel
    {
        public:
            ConsolePanel();
        protected:
            void on_draw() override;
    };
}