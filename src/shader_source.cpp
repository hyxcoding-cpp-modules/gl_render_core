#include "glcore/shader_source.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace glcore
{

// ========== 文件读取 ==========
bool readFile(const std::string& path, std::string& outContent)
{
    outContent.clear();
    if (path.empty())
    {
        return false;
    }

    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    if (file.bad())
    {
        return false;
    }

    outContent = buffer.str();
    return true;
}

ShaderSource fromFiles(const std::string& vertexPath,
                       const std::string& fragmentPath,
                       const std::string& geometryPath)
{
    ShaderSource source;
    if (!readFile(vertexPath, source.vertex))
    {
        source.vertex.clear();
    }
    if (!readFile(fragmentPath, source.fragment))
    {
        source.fragment.clear();
    }
    if (!geometryPath.empty() && !readFile(geometryPath, source.geometry))
    {
        source.geometry.clear();
    }
    return source;
}

ShaderSource
fromStrings(const std::string& vertex, const std::string& fragment, const std::string& geometry)
{
    ShaderSource source;
    source.vertex = vertex;
    source.fragment = fragment;
    source.geometry = geometry;
    return source;
}

// ========== 注释剥离 ==========
std::string stripComments(const std::string& source)
{
    std::string result;
    result.reserve(source.size());

    const std::size_t length = source.size();
    std::size_t i = 0;
    bool inDoubleQuote = false;
    bool inSingleQuote = false;

    while (i < length)
    {
        const char current = source[i];

        if (inDoubleQuote || inSingleQuote)
        {
            result.push_back(current);
            if (current == '\\' && i + 1 < length)
            {
                // 转义序列整体保留，避免 \" 被误判为字符串结束
                result.push_back(source[i + 1]);
                i += 2;
                continue;
            }
            if (inDoubleQuote && current == '"')
            {
                inDoubleQuote = false;
            }
            else if (inSingleQuote && current == '\'')
            {
                inSingleQuote = false;
            }
            ++i;
            continue;
        }

        if (current == '"')
        {
            inDoubleQuote = true;
            result.push_back(current);
            ++i;
            continue;
        }

        if (current == '\'')
        {
            inSingleQuote = true;
            result.push_back(current);
            ++i;
            continue;
        }

        if (current == '/' && i + 1 < length && source[i + 1] == '/')
        {
            // 行注释：跳到行尾，换行本身保留
            i += 2;
            while (i < length && source[i] != '\n')
            {
                ++i;
            }
            continue;
        }

        if (current == '/' && i + 1 < length && source[i + 1] == '*')
        {
            // 块注释：整体删除（含内部换行），未闭合时吃到末尾
            i += 2;
            while (i + 1 < length && !(source[i] == '*' && source[i + 1] == '/'))
            {
                ++i;
            }
            i = (i + 1 < length) ? i + 2 : length;
            continue;
        }

        result.push_back(current);
        ++i;
    }

    return result;
}

// ========== uniform 位置缓存 ==========
int ShaderUniformCache::getLocation(const std::string& name) const
{
    std::map<std::string, int>::const_iterator it = m_entries.find(name);
    if (it == m_entries.end())
    {
        return kInvalidUniformLocation;
    }
    return it->second;
}

void ShaderUniformCache::setLocation(const std::string& name, int location)
{
    m_entries[name] = location;
}

bool ShaderUniformCache::has(const std::string& name) const
{
    return m_entries.find(name) != m_entries.end();
}

void ShaderUniformCache::clear()
{
    m_entries.clear();
}

std::size_t ShaderUniformCache::size() const
{
    return m_entries.size();
}

std::vector<std::string> ShaderUniformCache::names() const
{
    std::vector<std::string> result;
    result.reserve(m_entries.size());
    for (std::map<std::string, int>::const_iterator it = m_entries.begin(); it != m_entries.end();
         ++it)
    {
        result.push_back(it->first);
    }
    // std::map 本身有序，这里再排一次以保证接口契约不依赖容器实现
    std::sort(result.begin(), result.end());
    return result;
}

}  // namespace glcore
