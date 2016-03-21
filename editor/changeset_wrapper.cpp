#include "editor/changeset_wrapper.hpp"
#include "editor/osm_feature_matcher.hpp"

#include "indexer/feature.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include "std/algorithm.hpp"
#include "std/sstream.hpp"

#include "private.h"

using editor::XMLFeature;

namespace
{
m2::RectD GetBoundingRect(vector<m2::PointD> const & geometry)
{
  m2::RectD rect;
  for (auto const & p : geometry)
  {
    auto const latLon = MercatorBounds::ToLatLon(p);
    rect.Add({latLon.lon, latLon.lat});
  }
  return rect;
}
}  // namespace

namespace pugi
{
string DebugPrint(xml_document const & doc)
{
  ostringstream stream;
  doc.print(stream, "  ");
  return stream.str();
}
}  // namespace pugi

namespace osm
{
ChangesetWrapper::ChangesetWrapper(TKeySecret const & keySecret,
                                   ServerApi06::TKeyValueTags const & comments) noexcept
  : m_changesetComments(comments), m_api(OsmOAuth::ServerAuth(keySecret))
{
}

ChangesetWrapper::~ChangesetWrapper()
{
  if (m_changesetId)
  {
    try
    {
      m_api.CloseChangeSet(m_changesetId);
    }
    catch (std::exception const & ex)
    {
      LOG(LWARNING, (ex.what()));
    }
  }
}

void ChangesetWrapper::LoadXmlFromOSM(ms::LatLon const & ll, pugi::xml_document & doc)
{
  auto const response = m_api.GetXmlFeaturesAtLatLon(ll.lat, ll.lon);
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(HttpErrorException, ("HTTP error", response, "with GetXmlFeaturesAtLatLon", ll));

  if (pugi::status_ok != doc.load(response.second.c_str()).status)
    MYTHROW(OsmXmlParseException, ("Can't parse OSM server response for GetXmlFeaturesAtLatLon request", response.second));
}

void ChangesetWrapper::LoadXmlFromOSM(m2::RectD const & rect, pugi::xml_document & doc)
{
  auto const response = m_api.GetXmlFeaturesInRect(rect);
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(HttpErrorException, ("HTTP error", response, "with GetXmlFeaturesInRect", rect));

  if (pugi::status_ok != doc.load(response.second.c_str()).status)
    MYTHROW(OsmXmlParseException, ("Can't parse OSM server response for GetXmlFeaturesInRect request", response.second));
}

XMLFeature ChangesetWrapper::GetMatchingNodeFeatureFromOSM(m2::PointD const & center)
{
  // Match with OSM node.
  ms::LatLon const ll = MercatorBounds::ToLatLon(center);
  pugi::xml_document doc;
  // Throws!
  LoadXmlFromOSM(ll, doc);

  pugi::xml_node const bestNode = GetBestOsmNode(doc, ll);
  if (bestNode.empty())
  {
    MYTHROW(OsmObjectWasDeletedException,
            ("OSM does not have any nodes at the coordinates", ll, ", server has returned:", doc));
  }

  return XMLFeature(bestNode);
}

XMLFeature ChangesetWrapper::GetMatchingAreaFeatureFromOSM(vector<m2::PointD> const & geometry)
{
  // TODO: Make two/four requests using points on inscribed rectagle.
  bool hasRelation = false;
  for (auto const & pt : geometry)
  {
    ms::LatLon const ll = MercatorBounds::ToLatLon(pt);
    pugi::xml_document doc;
    // Throws!
    LoadXmlFromOSM(ll, doc);

    if (doc.select_node("osm/relation"))
    {
      auto const rect = GetBoundingRect(geometry);
      LoadXmlFromOSM(rect, doc);
      hasRelation = true;
    }

    pugi::xml_node const bestWayOrRelation = GetBestOsmWayOrRelation(doc, geometry);
    if (!bestWayOrRelation)
    {
      if (hasRelation)
        break;
      continue;
    }

    if (strcmp(bestWayOrRelation.name(), "relation") == 0)
    {
      stringstream sstr;
      bestWayOrRelation.print(sstr);
      LOG(LDEBUG, ("Relation is the best match", sstr.str()));
      MYTHROW(RelationFeatureAreNotSupportedException,
              ("Got relation as the best matching."));
    }

    // TODO: rename to wayOrRelation when relations are handled.
    XMLFeature const way(bestWayOrRelation);
    ASSERT(way.IsArea(), ("Best way must be an area."));

    // AlexZ: TODO: Check that this way is really match our feature.
    // If we had some way to check it, why not to use it in selecting our feature?

    return way;
  }
  MYTHROW(OsmObjectWasDeletedException, ("OSM does not have any matching way for feature"));
}

void ChangesetWrapper::Create(XMLFeature node)
{
  if (m_changesetId == kInvalidChangesetId)
    m_changesetId = m_api.CreateChangeSet(m_changesetComments);

  // Changeset id should be updated for every OSM server commit.
  node.SetAttribute("changeset", strings::to_string(m_changesetId));
  // TODO(AlexZ): Think about storing/logging returned OSM ids.
  UNUSED_VALUE(m_api.CreateElement(node));
}

void ChangesetWrapper::Modify(XMLFeature node)
{
  if (m_changesetId == kInvalidChangesetId)
    m_changesetId = m_api.CreateChangeSet(m_changesetComments);

  // Changeset id should be updated for every OSM server commit.
  node.SetAttribute("changeset", strings::to_string(m_changesetId));
  m_api.ModifyElement(node);
}

void ChangesetWrapper::Delete(XMLFeature node)
{
  if (m_changesetId == kInvalidChangesetId)
    m_changesetId = m_api.CreateChangeSet(m_changesetComments);

  // Changeset id should be updated for every OSM server commit.
  node.SetAttribute("changeset", strings::to_string(m_changesetId));
  m_api.DeleteElement(node);
}

} // namespace osm
