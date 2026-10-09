#pragma once

#include <Rivet/panel/EditorPanel.hpp>

namespace Rivet
{
    class ViewportPanel : public EditorPanel
    {
        public:
            ViewportPanel();
        protected:
            void on_draw() override;
    };
}