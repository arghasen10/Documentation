/*
 * dash-helper.cc
 *
 *  Created on: 09-Apr-2020
 *      Author: abhijit
 */

#include "endash-helper.h"
#include "ns3/dash-request-handler.h"
#include "ns3/endash-video-player.h"
#include "ns3/http-server.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/mobility-model.h"

namespace ns3 {

EnDashServerHelper::EnDashServerHelper(uint16_t port) :
		HttpServerHelper(port) {
	SetAttribute("HttpRequestHandlerTypeId",
			TypeIdValue(DashRequestHandler::GetTypeId()));
}

} /* namespace ns3 */

namespace ns3 {

EnDashClientHelper::EnDashClientHelper(Address addr, uint16_t port) {
	m_factory.SetTypeId(EnDashVideoPlayer::GetTypeId());
	SetAttribute("RemoteAddress", AddressValue(Address(addr)));
	SetAttribute("RemotePort", UintegerValue(port));
}

void EnDashClientHelper::SetAttribute(std::string name,
		const AttributeValue &value) {
	m_factory.Set(name, value);
}


void
CourseChange (Ptr<const MobilityModel> model)
{
  Vector position = model->GetPosition ();
  Vector velo = model->GetVelocity ();
  std::cout << "CourseChange x = " << position.x << ", y = " << position.y << "\n";
  double vel = std::sqrt((velo.x *velo.x) + (velo.y * velo.y));
  std::cout << "Velocity: " << vel << std::endl; 
}

ApplicationContainer EnDashClientHelper::Install(NodeContainer nodes) const {
	ApplicationContainer apps;
	for (auto it = nodes.Begin(); it != nodes.End(); ++it) {
		apps.Add(InstallPriv(*it));
	}
	return apps;
}

ApplicationContainer EnDashClientHelper::Install(Ptr<Node> node) const {
	return ApplicationContainer(InstallPriv(node));
}

Ptr<Application> EnDashClientHelper::InstallPriv(Ptr<Node> node) const {
	Ptr<MobilityModel> mobmodel = node->GetObject<MobilityModel> ();
	mobmodel->TraceConnectWithoutContext("CourseChange", MakeCallback(&CourseChange));
	Ptr<Application> app = m_factory.Create<EnDashVideoPlayer>();
	node->AddApplication(app);

	return app;
}

} /* namespace ns3 */

