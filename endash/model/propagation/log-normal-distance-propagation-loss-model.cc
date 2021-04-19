/*
 * log-normal-distance-propagation-loss-model.cc
 *
 *  Created on: Sep 20, 2020
 *      Author: abhijit
 */

#include "log-normal-distance-propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include <cmath>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LogNormalDistancePropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED (LogNormalDistancePropagationLossModel);


TypeId
LogNormalDistancePropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LogNormalDistancePropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<LogNormalDistancePropagationLossModel> ()
    .AddAttribute ("Exponent",
                  "The exponent of the Path Loss propagation model",
                  DoubleValue (3.0),
                  MakeDoubleAccessor (&LogNormalDistancePropagationLossModel::m_exponent),
                  MakeDoubleChecker<double> ())
    .AddAttribute ("ReferenceDistance",
                  "The distance at which the reference loss is calculated (m)",
                  DoubleValue (1.0),
                  MakeDoubleAccessor (&LogNormalDistancePropagationLossModel::m_referenceDistance),
                  MakeDoubleChecker<double> ())
    .AddAttribute ("ReferenceLoss",
                  "The reference loss at reference distance (dB). (Default is Friis at 1m with 5.15 GHz)",
                  DoubleValue (46.6777),
                  MakeDoubleAccessor (&LogNormalDistancePropagationLossModel::m_referenceLoss),
                  MakeDoubleChecker<double> ())
    .AddAttribute ("GaussianSD",
                   "The Gaussian distribution standard deviation in dB",
                   StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=4.0]"),
                   MakePointerAccessor (&LogNormalDistancePropagationLossModel::m_gaussianRV),
                   MakePointerChecker<RandomVariableStream>())
    ;
  return tid;

}

LogNormalDistancePropagationLossModel::LogNormalDistancePropagationLossModel ()
{
}

void
LogNormalDistancePropagationLossModel::SetPathLossExponent (double n)
{
  m_exponent = n;
}
void
LogNormalDistancePropagationLossModel::SetReference (double referenceDistance, double referenceLoss)
{
  m_referenceDistance = referenceDistance;
  m_referenceLoss = referenceLoss;
}
double
LogNormalDistancePropagationLossModel::GetPathLossExponent (void) const
{
  return m_exponent;
}

double
LogNormalDistancePropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                                      Ptr<MobilityModel> a,
                                                      Ptr<MobilityModel> b) const
{
  double distance = a->GetDistanceFrom (b);
  if (distance <= m_referenceDistance)
    {
      return txPowerDbm - m_referenceLoss;
    }
  double pathLossDb = 10 * m_exponent * std::log10 (distance / m_referenceDistance);
  double fadingLossDb = m_gaussianRV->GetValue();
  double rxc = -m_referenceLoss - pathLossDb - fadingLossDb;
  NS_LOG_DEBUG ("distance=" << distance << "m, reference-attenuation=" << -m_referenceLoss << "dB, " <<
                "attenuation coefficient=" << rxc << "db");
  return txPowerDbm + rxc;
}

int64_t
LogNormalDistancePropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

} /* namespace ns3 */
