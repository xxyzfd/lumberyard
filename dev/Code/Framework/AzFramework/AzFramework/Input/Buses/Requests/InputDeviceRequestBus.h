/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    class InputDevice;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to query input devices for their associated input channels and state
    class InputDeviceRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId so that they are only
        //! handled by one input device that has connected to the bus using that unique id, or they
        //! can be broadcast to all input devices that have connected to the bus, regardless of id.
        //! Connected input devices are ordered by their local player index from lowest to highest.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ByIdAndOrdered;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests should be handled by only one input device connected to each id
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        typedef AZStd::recursive_mutex MutexType;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can be addressed to a specific InputDeviceId
        using BusIdType = InputDeviceId;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests are handled by connected devices in the order of local player index
        using BusIdOrderCompare = AZStd::less<BusIdType>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Alias for verbose container class
        using InputDeviceIdSet = AZStd::unordered_set<InputDeviceId>;
        using InputChannelIdSet = AZStd::unordered_set<InputChannelId>;
        using InputDeviceByIdMap = AZStd::unordered_map<InputDeviceId, const InputDevice*>;
        using InputChannelByIdMap = AZStd::unordered_map<InputChannelId, const InputChannel*>;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Finds a specific input device (convenience function)
        //! \param[in] deviceId Id of the input device to find
        //! \return Pointer to the input device if it was found, nullptr if it was not
        static const InputDevice* FindInputDevice(const InputDeviceId& deviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request the ids of all input channels (optionally those associated with an input device)
        //! that return custom data of a specific type (InputChannel::GetCustomData<CustomDataType>).
        //! \param[out] o_channelIds The set of input channel ids to return
        //! \param[in] deviceId (optional) Id of a specific input device to query for input channels
        //! \tparam CustomDataType Only consider input channels that return custom data of this type
        template<class CustomDataType>
        static void GetInputChannelIdsWithCustomDataOfType(InputChannelIdSet& o_channelIds,
                                                           const InputDeviceId* deviceId = nullptr);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Gets the input device that is uniquely identified by the InputDeviceId used to address
        //! the call to this EBus function. Calls to this EBus method should never be broadcast to
        //! all connected input devices, otherwise the device returned will effectively be random.
        //! \return Pointer to the input device if it exists, nullptr otherwise
        virtual const InputDevice* GetInputDevice() const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request the ids of all currently enabled input devices. This does not imply they are all
        //! connected, or even available on the current platform, just that they are enabled for the
        //! application (meaning they will generate input when available / connected to the system).
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices will add their id to o_deviceIds)
        //! - EBus<>::Event(id) (the given device will add its id to o_deviceIds - not very useful!)
        //!
        //! \param[out] o_deviceIds The set of input device ids to return
        virtual void GetInputDeviceIds(InputDeviceIdSet& o_deviceIds) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request a map of all currently enabled input devices by id. This does not imply they are
        //! connected, or even available on the current platform, just that they are enabled for the
        //! application (meaning they will generate input when available / connected to the system).
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices will add themselves to o_devicesById)
        //! - EBus<>::Event(id) (the given input device will add itself to o_devicesById)
        //!
        //! \param[out] o_devicesById The map of input devices (keyed by their id) to return
        virtual void GetInputDevicesById(InputDeviceByIdMap& o_devicesById) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request the ids of all input channels associated with an input device.
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices will add all their channel ids to o_channelIds)
        //! - EBus<>::Event(id) (the given device will add all of its channel ids to o_channelIds)
        //!
        //! \param[out] o_channelIds The set of input channel ids to return
        virtual void GetInputChannelIds(InputChannelIdSet& o_channelIds) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Request all input channels associated with an input device.
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices will add all their channels to o_channelsById)
        //! - EBus<>::Event(id) (the given device will add all of its channels to o_channelsById)
        //!
        //! \param[out] o_channelsById The map of input channels (keyed by their id) to return
        virtual void GetInputChannelsById(InputChannelByIdMap& o_channelsById) const = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Tick/update input devices.
        //!
        //! Can be called using either:
        //! - EBus<>::Broadcast (all input devices are ticked/updated)
        //! - EBus<>::Event(id) (the given device is ticked/updated)
        virtual void TickInputDevice() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputDeviceRequests() = default;
    };
    using InputDeviceRequestBus = AZ::EBus<InputDeviceRequests>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline const InputDevice* InputDeviceRequests::FindInputDevice(const InputDeviceId& deviceId)
    {
        const InputDevice* inputDevice = nullptr;
        InputDeviceRequestBus::EventResult(inputDevice, deviceId, &InputDeviceRequests::GetInputDevice);
        return inputDevice;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    template<class CustomDataType>
    inline void InputDeviceRequests::GetInputChannelIdsWithCustomDataOfType(
        InputChannelIdSet& o_channelIds,
        const InputDeviceId* deviceId)
    {
        InputChannelByIdMap inputChannelsById;
        if (deviceId)
        {
            InputDeviceRequestBus::Event(*deviceId, &InputDeviceRequests::GetInputChannelsById, inputChannelsById);
        }
        else
        {
            InputDeviceRequestBus::Broadcast(&InputDeviceRequests::GetInputChannelsById, inputChannelsById);
        }

        for (const auto& inputChannelById : inputChannelsById)
        {
            if (inputChannelById.second->GetCustomData<CustomDataType>() != nullptr)
            {
                o_channelIds.insert(inputChannelById.first);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Templated EBus interface used to create a custom implementation for a specific device type
    template<class InputDeviceType>
    class InputDeviceImplementationRequest : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for the EBus implementation of this interface
        using Bus = AZ::EBus<InputDeviceImplementationRequest<InputDeviceType>>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for the function type used to create the custom implementations
        using CreateFunctionType = typename InputDeviceType::Implementation*(*)(InputDeviceType&);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Create a custom implementation for all the existing instances of this input device type.
        //! Passing InputDeviceType::Implementation::Create as the argument will create the default
        //! device implementation, while passing nullptr will delete any existing implementation.
        //! \param[in] createFunction Pointer to the function that will create the implementation.
        virtual void CreateCustomImplementation(CreateFunctionType createFunction) = 0;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Templated EBus handler class that implements the InputDeviceImplementationRequest interface.
    //! To use this helper class your InputDeviceType class must posses all of the following traits,
    //! and they must all be accessible (either by being public or by making this helper a friend):
    //! - A nested InputDeviceType::Implementation class
    //! - A SetImplementation(AZStd::unique_ptr<InputDeviceType::Implementation>) function
    template<class InputDeviceType>
    class InputDeviceImplementationRequestHandler
        : public InputDeviceImplementationRequest<InputDeviceType>::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device that owns this handler
        AZ_INLINE InputDeviceImplementationRequestHandler(InputDeviceType& inputDevice)
            : m_inputDevice(inputDevice)
        {
            InputDeviceImplementationRequest<InputDeviceType>::Bus::Handler::BusConnect();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputDeviceImplementationRequestHandler);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Alias for the function type used to create the custom implementations
        using CreateFunctionType = typename InputDeviceType::Implementation*(*)(InputDeviceType&);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref InputDeviceImplementationRequest<InputDeviceType>::CreateCustomImplementation
        AZ_INLINE void CreateCustomImplementation(CreateFunctionType createFunction) override
        {
            AZStd::unique_ptr<typename InputDeviceType::Implementation> newImplementation;
            if (createFunction)
            {
                newImplementation.reset(createFunction(m_inputDevice));
            }
            m_inputDevice.SetImplementation(AZStd::move(newImplementation));
        }

    private:
        InputDeviceType& m_inputDevice; //!< Reference to the input device that owns this handler
    };
} // namespace AzFramework
