/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "core/svg/SVGCircleElement.h"

#include "core/layout/svg/LayoutSVGEllipse.h"
#include "core/svg/SVGLength.h"

namespace blink {

inline SVGCircleElement::SVGCircleElement(Document& document)
    : SVGGeometryElement(SVGNames::circleTag, document)
    , m_cx(SVGAnimatedLength::create(this, SVGNames::cxAttr, SVGLength::create(LengthModeWidth), AllowNegativeLengths))
    , m_cy(SVGAnimatedLength::create(this, SVGNames::cyAttr, SVGLength::create(LengthModeHeight), AllowNegativeLengths))
    , m_r(SVGAnimatedLength::create(this, SVGNames::rAttr, SVGLength::create(LengthModeOther), ForbidNegativeLengths))
{
    addToPropertyMap(m_cx);
    addToPropertyMap(m_cy);
    addToPropertyMap(m_r);
}

DEFINE_TRACE(SVGCircleElement)
{
    visitor->trace(m_cx);
    visitor->trace(m_cy);
    visitor->trace(m_r);
    SVGGeometryElement::trace(visitor);
}

DEFINE_NODE_FACTORY(SVGCircleElement)

bool SVGCircleElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        supportedAttributes.add(SVGNames::cxAttr);
        supportedAttributes.add(SVGNames::cyAttr);
        supportedAttributes.add(SVGNames::rAttr);
    }
    return supportedAttributes.contains<SVGAttributeHashTranslator>(attrName);
}

void SVGCircleElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    parseAttributeNew(name, value);
}

void SVGCircleElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGGeometryElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElement::InvalidationGuard invalidationGuard(this);

    bool isLengthAttribute = attrName == SVGNames::cxAttr
                          || attrName == SVGNames::cyAttr
                          || attrName == SVGNames::rAttr;

    if (isLengthAttribute)
        updateRelativeLengthsInformation();

    LayoutSVGShape* renderer = toLayoutSVGShape(this->renderer());
    if (!renderer)
        return;

    if (isLengthAttribute) {
        renderer->setNeedsShapeUpdate();
        markForLayoutAndParentResourceInvalidation(renderer);
        return;
    }

    ASSERT_NOT_REACHED();
}

bool SVGCircleElement::selfHasRelativeLengths() const
{
    return m_cx->currentValue()->isRelative()
        || m_cy->currentValue()->isRelative()
        || m_r->currentValue()->isRelative();
}

LayoutObject* SVGCircleElement::createRenderer(const LayoutStyle&)
{
    return new LayoutSVGEllipse(this);
}

}
