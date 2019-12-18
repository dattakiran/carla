// Copyright (c) 2019 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "BatchControlStage.h"

namespace carla {
namespace traffic_manager {

  BatchControlStage::BatchControlStage(
      std::string stage_name,
      std::shared_ptr<PlannerToControlMessenger> messenger,
      cc::Client &carla_client)
    : PipelineStage(stage_name),
      messenger(messenger),
      carla_client(carla_client),
      world(carla_client.GetWorld()) {

    // Initializing number of vehicles to zero in the beginning.
    number_of_vehicles = 0u;
  }

  BatchControlStage::~BatchControlStage() {}

  void BatchControlStage::Action() {

    // Looping over registered actors.
    for (uint64_t i = 0u; i < number_of_vehicles && data_frame != nullptr; ++i) {

      cr::VehicleControl vehicle_control;

      const PlannerToControlData &element = data_frame->at(i);
      const carla::ActorId actor_id = element.actor_id;
      vehicle_control.throttle = element.throttle;
      vehicle_control.brake = element.brake;
      vehicle_control.steer = element.steer;

      commands->at(i) = cr::Command::ApplyVehicleControl(actor_id, vehicle_control);
    }
  }

  void BatchControlStage::DataReceiver() {

    data_frame = messenger->Peek();

    // Allocating new containers for the changed number of registered vehicles.
    if (data_frame != nullptr &&
        number_of_vehicles != (*data_frame.get()).size()) {

      number_of_vehicles = static_cast<uint>((*data_frame.get()).size());
      // Allocating array for command batching.
      commands = std::make_shared<std::vector<cr::Command>>(number_of_vehicles);
    }

  }

  void BatchControlStage::DataSender() {

    messenger->Pop();

    if (commands != nullptr) {
      carla_client.ApplyBatch(*commands.get());
      world.Tick();
    }
  }

} // namespace traffic_manager
} // namespace carla
