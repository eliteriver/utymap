#ifndef FORMATS_SHAPE_SHAPEPARSER_HPP_INCLUDED
#define FORMATS_SHAPE_SHAPEPARSER_HPP_INCLUDED

#include "GeoCoordinate.hpp"
#include "entities/Element.hpp"
#include "formats/FormatTypes.hpp"
#include "shapefil.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

namespace utymap { namespace formats {

template<typename Visitor>
class ShapeParser
{
public:

    void parse(const std::string& path, Visitor& visitor)
    {
        SHPHandle shpFile = SHPOpen(path.c_str(), "rb");
        if (shpFile == NULL)
            throw std::domain_error("Cannot open shp file.");

        int shapeType, entityCount;
        double adfMinBound[4], adfMaxBound[4];
        SHPGetInfo(shpFile, &entityCount, &shapeType, adfMinBound, adfMaxBound);

        DBFHandle dbfFile = DBFOpen(path.c_str(), "rb");
        if (dbfFile == NULL)
            throw std::domain_error("Cannot open dbf file.");

        if (DBFGetFieldCount(dbfFile) == 0)
            throw std::domain_error("There are no fields in dbf table.");

        if (entityCount != DBFGetRecordCount(dbfFile))
            throw std::domain_error("dbf file has different entity count.");

        for (int k = 0; k < entityCount; k++)
        {
            SHPObject* shape = SHPReadObject(shpFile, k);
            if (shape == NULL)
                throw std::domain_error("Unable to read shape:" + to_string(k));

            visitShape(shape, parseTags(dbfFile, k), visitor);

            SHPDestroyObject(shape);
        }

        DBFClose(dbfFile);
        SHPClose(shpFile);
    }

private:
    // NOTE: Workaround due to MinGW g++ compiler
    template <typename T>
    inline std::string to_string(T const& value)
    {
        std::stringstream sstr;
        sstr << value;
        return sstr.str();
    }

    inline TagCollection parseTags(DBFHandle dbfFile, int k)
    {
        char title[12];
        int fieldCount = DBFGetFieldCount(dbfFile);
        TagCollection tags;
        tags.reserve(fieldCount);
        for (int i = 0; i < fieldCount; i++)
        {
            if (DBFIsAttributeNULL(dbfFile, k, i))
                continue;

            utymap::formats::Tag tag;
            int width, decimals;
            DBFFieldType eType = DBFGetFieldInfo(dbfFile, i, title, &width, &decimals);
            tag.key = std::string(title);
            {
                switch (eType)
                {
                    case FTString:
                        tag.value = DBFReadStringAttribute(dbfFile, k, i);
                        break;
                    case FTInteger:
                        tag.value = to_string(DBFReadIntegerAttribute(dbfFile, k, i));
                        break;
                    case FTDouble:
                        tag.value = to_string(DBFReadDoubleAttribute(dbfFile, k, i));
                        break;
                    default:
                        break;
                }
            }
            tags.push_back(tag);
        }
        return std::move(tags);
    }

    inline void visitShape(SHPObject* shape, TagCollection& tags, Visitor& visitor)
    {
        switch (shape->nSHPType)
        {
            case SHPT_POINT:
            case SHPT_POINTM:
            case SHPT_POINTZ:
                visitPoint(shape, tags, visitor);
                break;
            case SHPT_ARC:
            case SHPT_ARCZ:
            case SHPT_ARCM:
                visitArc(shape, tags, visitor);
                break;
            case SHPT_POLYGON:
            case SHPT_POLYGONZ:
            case SHPT_POLYGONM:
                visitPolygon(shape, tags, visitor);
                break;
            case SHPT_MULTIPOINT:
            case SHPT_MULTIPOINTZ:
            case SHPT_MULTIPOINTM:
            case SHPT_MULTIPATCH:
                std::cout << "Unsupported shape type:" << SHPTypeName(shape->nSHPType);
                break;
            default:
                std::cout << "Unknown shape type:" << SHPTypeName(shape->nSHPType);
                break;
        }
    }

    inline void visitPoint(SHPObject* shape, TagCollection& tags, Visitor& visitor)
    {
        visitor.visitNode(utymap::GeoCoordinate(shape->padfX[0], shape->padfY[0]), tags);
    }

    inline void visitArc(SHPObject* shape, TagCollection& tags, Visitor& visitor)
    {
        std::vector<utymap::GeoCoordinate> coordinates;
        coordinates.reserve(shape->nVertices);
        for (int j = 0; j < shape->nVertices; j++)
            coordinates.push_back(utymap::GeoCoordinate(shape->padfX[j], shape->padfY[j]));
    }

    inline void visitPolygon(SHPObject* shape, TagCollection& tags, Visitor& visitor)
    {
        std::vector<utymap::GeoCoordinate> coordinates;
        coordinates.reserve(shape->nVertices);
        for (int j = 0, iPart = 1; j < shape->nVertices; ++j) {
            if (iPart < shape->nParts && shape->panPartStart[iPart] == j) {
                // new part is started
            }
        }
    }

};

}}

#endif  // FORMATS_SHAPE_SHAPEPARSER_HPP_INCLUDED
