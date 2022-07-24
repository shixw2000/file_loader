#include"string.h"
#include"ictx.h"
#include"istate.h"
#include"msgcenter.h"


I_Ctx::I_Ctx() {
    m_type = -1;
    m_has_chg = FALSE;
    m_factory = NULL;
    m_state = NULL; 
}

I_Ctx::~I_Ctx() {
}

Int32 I_Ctx::start() { 
    m_type = 0;
    m_has_chg = FALSE;

    if (NULL != m_factory) {
        m_state = m_factory->creat(m_type); 

        if (NULL != m_state) {
            m_state->prepare(this); 
            while (m_has_chg) {
                m_state->post(); 
                m_factory->release(m_state);

                m_state = m_factory->creat(m_type); 
                m_has_chg = FALSE; 

                if (NULL != m_state) {
                    m_state->prepare(this); 
                }
            }

            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

Void I_Ctx::stop() {
    if (NULL != m_state) {
        m_state->post(); 
        m_factory->release(m_state);

        m_state = NULL;
        m_type = -1;
        m_has_chg = FALSE;
    } 
}

Void I_Ctx::setFactory(I_Factory* factory) {
    m_factory = factory;
}

Void I_Ctx::setState(Int32 type) {
    if (type != m_type) {
        m_type = type;
        
        if (!m_has_chg) {
            m_has_chg = TRUE;
        } 
    }
}

void I_Ctx::procEvent(Void* msg) {

    if (NULL != m_state) {
        m_state->process(msg);
        
        while (m_has_chg) {
            m_state->post(); 
            m_factory->release(m_state);

            m_state = m_factory->creat(m_type);
            m_has_chg = FALSE;

            if (NULL != m_state) {
                m_state->prepare(this); 
            }
        }
    } else {
        MsgCenter::freeMsg(msg);
    }
}


