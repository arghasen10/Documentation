/*
 * log-normal-distance-propagation-loss-model.h
 *
 *  Created on: Sep 20, 2020
 *      Author: abhijit
 */

#ifndef SRC_SPDASH_MODEL_PROPAGATION_LOG_NORMAL_DISTANCE_PROPAGATION_LOSS_MODEL_H_
#define SRC_SPDASH_MODEL_PROPAGATION_LOG_NORMAL_DISTANCE_PROPAGATION_LOSS_MODEL_H_
#include <ns3/propagation-loss-model.h>

namespace ns3 {

class LogNormalDistancePropagationLossModel : public PropagationLossModel
{
public:
  static TypeId GetTypeId (void);
  LogNormalDistancePropagationLossModel ();

  void SetPathLossExponent (double n);
  double GetPathLossExponent (void) const;

  void SetReference (double referenceDistance, double referenceLoss);

private:
  LogNormalDistancePropagationLossModel (const LogNormalDistancePropagationLossModel &);
  LogNormalDistancePropagationLossModel & operator = (const LogNormalDistancePropagationLossModel &);

  virtual double DoCalcRxPower (double txPowerDbm,
                                Ptr<MobilityModel> a,
                                Ptr<MobilityModel> b) const;
  virtual int64_t DoAssignStreams (int64_t stream);

  static Ptr<PropagationLossModel> CreateDefaultReference (void);

  double m_exponent;
  double m_referenceDistance;
  double m_referenceLoss;
  Ptr<RandomVariableStream> m_gaussianRV;
};

} /* namespace ns3 */

#endif /* SRC_SPDASH_MODEL_PROPAGATION_LOG_NORMAL_DISTANCE_PROPAGATION_LOSS_MODEL_H_ */
