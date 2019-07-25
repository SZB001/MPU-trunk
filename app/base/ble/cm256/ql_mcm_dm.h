#ifndef __QL_MCM_DM_H__
#define __QL_MCM_DM_H__




typedef uint32 dm_client_handle_type;


int QL_MCM_DM_Client_Init(atc_client_handle_type  *ph_dm);

/* Stop getting coordinates */
int QL_MCM_DM_Client_Deinit(dm_client_handle_type h_dm);

E_QL_ERROR_CODE_T QL_MCM_DM_GetAirplaneMode
(
    dm_client_handle_type                   h_dm,
    E_QL_MCM_DM_AIRPLANE_MODE_TYPE_T        *pe_airplane_mode  ///< [OUT] Radio mode
);

E_QL_ERROR_CODE_T QL_MCM_DM_SetAirplaneMode
(
    dm_client_handle_type                   h_dm,
    E_QL_MCM_DM_AIRPLANE_MODE_TYPE_T        e_airplane_mode  ///< [IN] Airplane mode
);

E_QL_ERROR_CODE_T QL_MCM_DM_SetAirplaneModeChgInd
(
    dm_client_handle_type       h_dm,
    uint32_t                    ind_onoff  ///< [IN] 0: indication off, 1: on
);


#endif // __QL_MCM_DM_H__

