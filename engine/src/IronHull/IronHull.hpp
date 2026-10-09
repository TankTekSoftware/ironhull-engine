#pragma once

#include <IronHull/core/Application.hpp>

#include <IronHull/input/Input.hpp>

#include <IronHull/render/RenderPass.hpp>

#include <IronHull/scene/Scene.hpp>
#include <IronHull/scene/SceneManager.hpp>

#include <IronHull/asset/AssetRegistry.hpp>

#include <IronHull/io/FileSystem.hpp>
#include <IronHull/io/SaveFile.hpp>

#include <IronHull/script/LuaVM.hpp>

#include <IronHull/entity/Entity.hpp>
#include <IronHull/entity/EntityDefinition.hpp>
#include <IronHull/entity/EntityRegistry.hpp>
#include <IronHull/entity/EntityWorld.hpp>
#include <IronHull/entity/PropertyValue.hpp>
#include <IronHull/entity/base/BaseProperties.hpp>

#include <IronHull/geometry/Bounds.hpp>
#include <IronHull/geometry/ConvexVolume.hpp>
#include <IronHull/geometry/Plane.hpp>
#include <IronHull/geometry/Winding.hpp>

#include <IronHull/map/BspFile.hpp>
#include <IronHull/map/CompiledMap.hpp>
#include <IronHull/map/Map.hpp>
#include <IronHull/map/MapFile.hpp>
#include <IronHull/map/MapSpace.hpp>

// The map compiler is included here too, rather than only by the ihbsp tool, so the editor
// can compile the map it has open without writing a file first.
#include <IronHull/map/compile/MapCompiler.hpp>

#include <IronHull/utils/ColorUtils.hpp>
#include <IronHull/utils/DataUtils.hpp>

#include <raylib.h>
#include <raymath.h>
#include <imgui.h>
