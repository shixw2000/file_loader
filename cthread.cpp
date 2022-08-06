#include"cthread.h"


CThread::CThread() {
    m_element = NULL;
    m_running = FALSE;
}

CThread::~CThread() {
}

Int32 CThread::start() {
    Int32 ret = 0; 
    
    m_running = TRUE; 

    while (m_running && 0 == ret) {
        ret = m_element->run();
    }
    
    return ret;
}

Void CThread::stop() {
    m_running = FALSE; 
}

Void CThread::setParam(I_Element* element) {
    m_element = element;
}

