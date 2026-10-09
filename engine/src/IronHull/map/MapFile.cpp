#include <IronHull/map/MapFile.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <raymath.h>

namespace IronHull
{
    namespace
    {
        // --- TEXTURE AXIS DERIVATION --- //

        // The six candidate texture projections, one per world-axis-aligned surface, as
        // triples of (surface normal, u axis, v axis). A face picks the entry whose normal it
        // most closely faces, which is what makes a plain Quake .map (storing no axes of its
        // own) project its texture along the wall rather than smeared across it.
        //
        // These are in Z-up map space, matching the coordinates in the file.
        const Vector3 BASE_TEXTURE_AXES[18] = {
            { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f },  // floor
            { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, // ceiling
            { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f },  // west wall
            { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, // east wall
            { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f },  // south wall
            { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, // north wall
        };

        void texture_axes_from_plane(const Plane& plane, Vector3& u_axis, Vector3& v_axis)
        {
            int best = 0;
            float best_dot = -1.0f;

            for (int index = 0; index < 6; ++index) {
                float dot = Vector3DotProduct(plane.normal, BASE_TEXTURE_AXES[index * 3]);
                if (dot > best_dot) {
                    best_dot = dot;
                    best = index;
                }
            }

            u_axis = BASE_TEXTURE_AXES[best * 3 + 1];
            v_axis = BASE_TEXTURE_AXES[best * 3 + 2];
        }

        // Spins the two texture axes about the face normal. Because the axes picked above are
        // always world-axis-aligned, exactly two of the three components take part in the
        // rotation and the third is left alone - which is why this indexes components rather
        // than building a rotation matrix.
        void rotate_texture_axes(Vector3& u_axis, Vector3& v_axis, float rotation)
        {
            float sine = 0.0f;
            float cosine = 1.0f;

            // The quarter turns are spelled out so a texture rotated by a right angle stays
            // perfectly grid-aligned instead of picking up sin/cos rounding error.
            if (rotation == 0.0f) {
                sine = 0.0f;
                cosine = 1.0f;
            } else if (rotation == 90.0f) {
                sine = 1.0f;
                cosine = 0.0f;
            } else if (rotation == 180.0f) {
                sine = 0.0f;
                cosine = -1.0f;
            } else if (rotation == 270.0f) {
                sine = -1.0f;
                cosine = 0.0f;
            } else {
                float radians = rotation * DEG2RAD;
                sine = std::sin(radians);
                cosine = std::cos(radians);
            }

            int s_index = u_axis.x != 0.0f ? 0 : (u_axis.y != 0.0f ? 1 : 2);
            int t_index = v_axis.x != 0.0f ? 0 : (v_axis.y != 0.0f ? 1 : 2);

            Vector3* axes[2] = { &u_axis, &v_axis };

            for (int index = 0; index < 2; ++index) {
                float* components[3] = { &axes[index]->x, &axes[index]->y, &axes[index]->z };

                float s = cosine * *components[s_index] - sine * *components[t_index];
                float t = sine * *components[s_index] + cosine * *components[t_index];

                *components[s_index] = s;
                *components[t_index] = t;
            }
        }

        // --- LEXER --- //

        enum class TokenKind
        {
            END,
            PUNCT,
            STRING,
            WORD,
        };

        struct Token
        {
            TokenKind kind = TokenKind::END;
            std::string text;
            int line = 0;

            bool is_punct(char character) const
            {
                return this->kind == TokenKind::PUNCT && this->text.size() == 1 && this->text[0] == character;
            }

            // Both a quoted value and a bare word read as text; which one the file used
            // never carries meaning, only whether it contained spaces.
            bool is_text() const
            {
                return this->kind == TokenKind::STRING || this->kind == TokenKind::WORD;
            }
        };

        bool looks_numeric(const std::string& text)
        {
            if (text.empty()) {
                return false;
            }

            char first = text[0];
            return (first >= '0' && first <= '9') || first == '-' || first == '+' || first == '.';
        }

        class Lexer
        {
            private:
                const std::string& text;
                size_t position = 0;
                int line = 1;

                bool peeked = false;
                Token peeked_token;

            public:
                explicit Lexer(const std::string& source) : text(source) { }

            public:
                const Token& peek()
                {
                    if (!this->peeked) {
                        this->peeked_token = this->read();
                        this->peeked = true;
                    }

                    return this->peeked_token;
                }

                Token next()
                {
                    if (this->peeked) {
                        this->peeked = false;
                        return this->peeked_token;
                    }

                    return this->read();
                }

                int current_line() const
                {
                    return this->line;
                }

            private:
                void skip_ignorable()
                {
                    while (this->position < this->text.size()) {
                        char character = this->text[this->position];

                        if (character == '\n') {
                            ++this->line;
                            ++this->position;
                            continue;
                        }

                        if (character == ' ' || character == '\t' || character == '\r' || character == '\f' || character == '\v') {
                            ++this->position;
                            continue;
                        }

                        // Editors label every entity and brush with a "// entity 3" comment,
                        // so these turn up throughout a real map file, not just at the top.
                        if (character == '/' && this->position + 1 < this->text.size() && this->text[this->position + 1] == '/') {
                            while (this->position < this->text.size() && this->text[this->position] != '\n') {
                                ++this->position;
                            }
                            continue;
                        }

                        break;
                    }
                }

                Token read()
                {
                    this->skip_ignorable();

                    Token token;
                    token.line = this->line;

                    if (this->position >= this->text.size()) {
                        token.kind = TokenKind::END;
                        return token;
                    }

                    char character = this->text[this->position];

                    if (character == '{' || character == '}' || character == '(' || character == ')' || character == '[' || character == ']') {
                        token.kind = TokenKind::PUNCT;
                        token.text.assign(1, character);
                        ++this->position;
                        return token;
                    }

                    if (character == '"') {
                        ++this->position;
                        token.kind = TokenKind::STRING;

                        while (this->position < this->text.size() && this->text[this->position] != '"') {
                            if (this->text[this->position] == '\n') {
                                throw MapParseError(token.line, "unterminated quoted string");
                            }

                            token.text.push_back(this->text[this->position]);
                            ++this->position;
                        }

                        if (this->position >= this->text.size()) {
                            throw MapParseError(token.line, "unterminated quoted string");
                        }

                        ++this->position;
                        return token;
                    }

                    token.kind = TokenKind::WORD;

                    while (this->position < this->text.size()) {
                        char current = this->text[this->position];

                        if (current == ' ' || current == '\t' || current == '\r' || current == '\n' || current == '"'
                            || current == '{' || current == '}' || current == '(' || current == ')' || current == '[' || current == ']') {
                            break;
                        }

                        // A single slash is part of the word - texture names are written as
                        // "walls/brick" - but a doubled one starts a comment.
                        if (current == '/' && this->position + 1 < this->text.size() && this->text[this->position + 1] == '/') {
                            break;
                        }

                        token.text.push_back(current);
                        ++this->position;
                    }

                    if (token.text.empty()) {
                        throw MapParseError(token.line, std::string("unexpected character '") + character + "'");
                    }

                    return token;
                }
        };

        // --- PARSER --- //

        Token expect_punct(Lexer& lexer, char character)
        {
            Token token = lexer.next();

            if (!token.is_punct(character)) {
                throw MapParseError(token.line, std::string("expected '") + character + "' but found '" + token.text + "'");
            }

            return token;
        }

        Token expect_text(Lexer& lexer, const char* what)
        {
            Token token = lexer.next();

            if (!token.is_text()) {
                throw MapParseError(token.line, std::string("expected ") + what + " but found '" + token.text + "'");
            }

            return token;
        }

        float expect_number(Lexer& lexer, const char* what)
        {
            Token token = lexer.next();

            if (!token.is_text() || !looks_numeric(token.text)) {
                throw MapParseError(token.line, std::string("expected ") + what + " but found '" + token.text + "'");
            }

            try {
                return std::stof(token.text);
            } catch (const std::exception&) {
                throw MapParseError(token.line, "'" + token.text + "' is not a number");
            }
        }

        Vector3 expect_point(Lexer& lexer)
        {
            expect_punct(lexer, '(');

            Vector3 point;
            point.x = expect_number(lexer, "an x coordinate");
            point.y = expect_number(lexer, "a y coordinate");
            point.z = expect_number(lexer, "a z coordinate");

            expect_punct(lexer, ')');

            return point;
        }

        MapFace parse_face(Lexer& lexer)
        {
            MapFace face;
            face.points[0] = expect_point(lexer);
            face.points[1] = expect_point(lexer);
            face.points[2] = expect_point(lexer);

            Token name = expect_text(lexer, "a texture name");
            face.texture.name = name.text;

            if (lexer.peek().is_punct('[')) {
                // Valve 220: the axes are written out, so nothing has to be inferred.
                expect_punct(lexer, '[');
                face.texture.u_axis.x = expect_number(lexer, "a u axis x");
                face.texture.u_axis.y = expect_number(lexer, "a u axis y");
                face.texture.u_axis.z = expect_number(lexer, "a u axis z");
                face.texture.u_shift = expect_number(lexer, "a u shift");
                expect_punct(lexer, ']');

                expect_punct(lexer, '[');
                face.texture.v_axis.x = expect_number(lexer, "a v axis x");
                face.texture.v_axis.y = expect_number(lexer, "a v axis y");
                face.texture.v_axis.z = expect_number(lexer, "a v axis z");
                face.texture.v_shift = expect_number(lexer, "a v shift");
                expect_punct(lexer, ']');

                face.texture.rotation = expect_number(lexer, "a texture rotation");
                face.texture.u_scale = expect_number(lexer, "a u scale");
                face.texture.v_scale = expect_number(lexer, "a v scale");
            } else {
                // Plain Quake: offset, rotation and scale only. Derive the axes from the
                // face normal and fold the rotation into them so that every face this parser
                // produces is in the explicit-axis form.
                face.texture.u_shift = expect_number(lexer, "a u shift");
                face.texture.v_shift = expect_number(lexer, "a v shift");
                face.texture.rotation = expect_number(lexer, "a texture rotation");
                face.texture.u_scale = expect_number(lexer, "a u scale");
                face.texture.v_scale = expect_number(lexer, "a v scale");

                texture_axes_from_plane(face.plane(), face.texture.u_axis, face.texture.v_axis);
                rotate_texture_axes(face.texture.u_axis, face.texture.v_axis, face.texture.rotation);
            }

            // Quake 2 and 3 append contents/surface/value flags to the face line. They have
            // no equivalent here (contents come from the texture name, see BrushGeometry), so
            // swallow any trailing numbers still on this line to keep such a map loadable.
            int face_line = lexer.peek().line;
            while (lexer.peek().kind == TokenKind::WORD && lexer.peek().line == face_line && looks_numeric(lexer.peek().text)) {
                lexer.next();
            }

            if (face.texture.u_scale == 0.0f) {
                face.texture.u_scale = 1.0f;
            }

            if (face.texture.v_scale == 0.0f) {
                face.texture.v_scale = 1.0f;
            }

            return face;
        }

        MapBrush parse_brush(Lexer& lexer)
        {
            MapBrush brush;

            while (true) {
                const Token& token = lexer.peek();

                if (token.kind == TokenKind::END) {
                    throw MapParseError(token.line, "unexpected end of file inside a brush");
                }

                if (token.is_punct('}')) {
                    lexer.next();
                    break;
                }

                brush.faces.push_back(parse_face(lexer));
            }

            if (brush.faces.size() < 4) {
                throw MapParseError(lexer.current_line(), "a brush needs at least 4 faces to enclose a volume, found "
                    + std::to_string(brush.faces.size()));
            }

            return brush;
        }

        MapConnection parse_connection(Lexer& lexer)
        {
            MapConnection connection;
            connection.output = expect_text(lexer, "an output name").text;
            connection.target = expect_text(lexer, "a target entity name").text;
            connection.input = expect_text(lexer, "an input name").text;
            connection.parameter = expect_text(lexer, "an input parameter").text;
            connection.delay = expect_number(lexer, "a delay");

            float times = expect_number(lexer, "a times-to-fire count");
            connection.times_to_fire = static_cast<int>(times);

            return connection;
        }

        void parse_connections(Lexer& lexer, MapEntity& entity)
        {
            expect_punct(lexer, '{');

            while (true) {
                const Token& token = lexer.peek();

                if (token.kind == TokenKind::END) {
                    throw MapParseError(token.line, "unexpected end of file inside a connections block");
                }

                if (token.is_punct('}')) {
                    lexer.next();
                    break;
                }

                entity.connections.push_back(parse_connection(lexer));
            }
        }

        MapEntity parse_entity(Lexer& lexer)
        {
            MapEntity entity;

            while (true) {
                Token token = lexer.peek();

                if (token.kind == TokenKind::END) {
                    throw MapParseError(token.line, "unexpected end of file inside an entity");
                }

                if (token.is_punct('}')) {
                    lexer.next();
                    break;
                }

                if (token.is_punct('{')) {
                    lexer.next();
                    entity.brushes.push_back(parse_brush(lexer));
                    continue;
                }

                if (token.kind == TokenKind::WORD && token.text == "connections") {
                    lexer.next();
                    parse_connections(lexer, entity);
                    continue;
                }

                Token key = expect_text(lexer, "a property name");
                Token value = expect_text(lexer, "a property value");

                entity.keyvalues.push_back(MapKeyValue{ key.text, value.text });
            }

            return entity;
        }

        // --- WRITER --- //

        // Prints a float with as few digits as will read back as the same value. Map
        // coordinates sit on an integer grid and come out as "16" rather than "16.000000",
        // while a texture axis that genuinely needs the precision still round trips exactly.
        std::string format_float(float value)
        {
            char buffer[64];

            std::snprintf(buffer, sizeof(buffer), "%.6g", static_cast<double>(value));
            if (std::strtof(buffer, nullptr) == value) {
                return std::string(buffer);
            }

            std::snprintf(buffer, sizeof(buffer), "%.9g", static_cast<double>(value));
            return std::string(buffer);
        }

        std::string format_point(Vector3 point)
        {
            return "( " + format_float(point.x) + " " + format_float(point.y) + " " + format_float(point.z) + " )";
        }

        void write_face(std::string& out, const MapFace& face)
        {
            out += format_point(face.points[0]);
            out += " ";
            out += format_point(face.points[1]);
            out += " ";
            out += format_point(face.points[2]);
            out += " ";
            out += face.texture.name;

            out += " [ " + format_float(face.texture.u_axis.x)
                + " " + format_float(face.texture.u_axis.y)
                + " " + format_float(face.texture.u_axis.z)
                + " " + format_float(face.texture.u_shift) + " ]";

            out += " [ " + format_float(face.texture.v_axis.x)
                + " " + format_float(face.texture.v_axis.y)
                + " " + format_float(face.texture.v_axis.z)
                + " " + format_float(face.texture.v_shift) + " ]";

            out += " " + format_float(face.texture.rotation)
                + " " + format_float(face.texture.u_scale)
                + " " + format_float(face.texture.v_scale);

            out += "\n";
        }
    }

    MapParseError::MapParseError(int line, const std::string& message)
        : std::runtime_error("MapFile: line " + std::to_string(line) + ": " + message)
    {
        this->line_number = line;
    }

    int MapParseError::line() const
    {
        return this->line_number;
    }

    Plane MapFace::plane() const
    {
        return Plane::from_points(this->points[0], this->points[1], this->points[2]);
    }

    const std::string* MapEntity::find(const std::string& key) const
    {
        for (const MapKeyValue& pair : this->keyvalues) {
            if (pair.key == key) {
                return &pair.value;
            }
        }

        return nullptr;
    }

    std::string MapEntity::get(const std::string& key, const std::string& fallback) const
    {
        const std::string* value = this->find(key);
        return value != nullptr ? *value : fallback;
    }

    void MapEntity::set(const std::string& key, const std::string& value)
    {
        for (MapKeyValue& pair : this->keyvalues) {
            if (pair.key == key) {
                pair.value = value;
                return;
            }
        }

        this->keyvalues.push_back(MapKeyValue{ key, value });
    }

    std::string MapEntity::classname() const
    {
        return this->get("classname");
    }

    bool MapEntity::is_worldspawn() const
    {
        return this->classname() == "worldspawn";
    }

    MapFile MapFile::parse(const std::string& text)
    {
        Lexer lexer(text);
        MapFile map;

        while (true) {
            Token token = lexer.peek();

            if (token.kind == TokenKind::END) {
                break;
            }

            if (!token.is_punct('{')) {
                throw MapParseError(token.line, "expected '{' to open an entity but found '" + token.text + "'");
            }

            lexer.next();
            map.entities.push_back(parse_entity(lexer));
        }

        return map;
    }

    std::string MapFile::write() const
    {
        std::string out;

        for (size_t entity_index = 0; entity_index < this->entities.size(); ++entity_index) {
            const MapEntity& entity = this->entities[entity_index];

            out += "// entity " + std::to_string(entity_index) + "\n{\n";

            for (const MapKeyValue& pair : entity.keyvalues) {
                out += "\"" + pair.key + "\" \"" + pair.value + "\"\n";
            }

            if (!entity.connections.empty()) {
                out += "connections\n{\n";

                for (const MapConnection& connection : entity.connections) {
                    out += "\"" + connection.output + "\""
                        + " \"" + connection.target + "\""
                        + " \"" + connection.input + "\""
                        + " \"" + connection.parameter + "\""
                        + " " + format_float(connection.delay)
                        + " " + std::to_string(connection.times_to_fire)
                        + "\n";
                }

                out += "}\n";
            }

            for (size_t brush_index = 0; brush_index < entity.brushes.size(); ++brush_index) {
                out += "// brush " + std::to_string(brush_index) + "\n{\n";

                for (const MapFace& face : entity.brushes[brush_index].faces) {
                    write_face(out, face);
                }

                out += "}\n";
            }

            out += "}\n";
        }

        return out;
    }

    const MapEntity* MapFile::worldspawn() const
    {
        for (const MapEntity& entity : this->entities) {
            if (entity.is_worldspawn()) {
                return &entity;
            }
        }

        return nullptr;
    }
}
