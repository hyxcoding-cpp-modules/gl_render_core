#include "glcore/math.hpp"

#include <sstream>

namespace glcore
{

// ========== 文本输出 ==========
std::string toString(const glm::vec3& value)
{
    std::ostringstream os;
    os << '(' << value.x << ',' << value.y << ',' << value.z << ')';
    return os.str();
}

std::string toString(const glm::mat4& value)
{
    std::ostringstream os;
    for (int row = 0; row < 4; ++row)
    {
        os << "[ ";
        for (int col = 0; col < 4; ++col)
        {
            // glm 为列主序：value[col] 是第 col 列，再取该列的第 row 个分量
            os << value[col][row];
            if (col != 3)
            {
                os << " , ";
            }
        }
        os << " ]";
        if (row != 3)
        {
            os << '\n';
        }
    }
    return os.str();
}

}  // namespace glcore
