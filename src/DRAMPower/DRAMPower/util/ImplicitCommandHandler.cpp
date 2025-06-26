#include "ImplicitCommandHandler.h"

namespace DRAMPower {

ImplicitCommandHandler::ImplicitCommandHandler()
    : m_implicitCommandList(std::make_shared<implicitCommandList_t>())
{}

ImplicitCommandHandler::Inserter::Inserter(const std::shared_ptr<implicitCommandList_t>& implicitCommandList)
    : m_implicitCommandList(implicitCommandList)
{}

std::size_t ImplicitCommandHandler::Inserter::implicitCommandCount() const{
    if (m_implicitCommandList.expired()) {
        return 0; // No implicit command list available
    }
    return m_implicitCommandList.lock()->size();
}

ImplicitCommandHandler::Inserter_t ImplicitCommandHandler::createInserter() {
    return Inserter_t (m_implicitCommandList);
}

void ImplicitCommandHandler::processImplicitCommandQueue(timestamp_t timestamp, timestamp_t &last_command_time) {
    // Process implicit command list
    while (!m_implicitCommandList->empty() && m_implicitCommandList->front().first <= timestamp) {
        // Execute implicit command functor
        auto& [i_timestamp, i_implicitCommand] = m_implicitCommandList->front();
        i_implicitCommand();
        last_command_time = i_timestamp;
        m_implicitCommandList->pop_front();
    }
}

std::size_t ImplicitCommandHandler::implicitCommandCount() const {
    return m_implicitCommandList->size();
}

} // namespace DRAMPower