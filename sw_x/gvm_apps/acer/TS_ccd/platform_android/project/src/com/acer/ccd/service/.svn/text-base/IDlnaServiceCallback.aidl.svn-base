package com.acer.ccd.service;

/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

interface IDlnaServiceCallback {
	/**
     * Callback function for M-search of DMS/DMR list
     * @param command request command from DMP
     * @param total DMS counts
     * @param errCode refer to CBCommand.ErrorID
     */
	void devNotifyReceived(int command, int total, int errCode); 

	/**
     * Callback function for DMS related command
     * @param command request command from DMP
     * @param total item counts
     * @param tableId sepcify which table should be update
     * @param uuid specify DMS's uuid
     * @param errCode refer to CBCommand.ErrorID
     */
	void dmsActionPerformed(int command, int total, int tableId, String uuid, int errCode);

	/**
     * Callback function for DMC related command
     * @param command request command from DMC
     * @param uuid specify DMR's uuid
     * @param errCode refer to CBCommand.ErrorID
     */
    void dmrActionPerformed(int command, String uuid, int errCode);
}