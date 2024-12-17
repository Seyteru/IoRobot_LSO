#!/bin/bash

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a deployer.log
}

> deployer.log
log "--- Starting deployment ---"


SCRIPT_PATH="$(readlink -f "$0")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

CONFIG_FILE="deployer-config.cfg"

if [[ ! -f "$SCRIPT_DIR/$CONFIG_FILE" ]]; then
    log "ERROR: Configuration file '$CONFIG_FILE' doesn't exists."
    exit 1
fi
log "Configuration file '$CONFIG_FILE' successfully loaded."

source "$SCRIPT_DIR/$CONFIG_FILE"

PRIVATE_KEY_PATH="$SCRIPT_DIR/$private_key"

if [[ -z "$private_key" || -z "$distribution" || -z "$ip_address" || -z "$folder_name" ]]; then
    log "ERROR: One or more variables in configuration file are missing."
    exit 1
fi
log "Configuration variables successfully verified."

if [[ ! -f "$PRIVATE_KEY_PATH" ]]; then
    log "ERROR: Private key not found in $PRIVATE_KEY_PATH"
    exit 1
fi
log "Private key found in $PRIVATE_KEY_PATH"

log "Zipping package..."
zip -r "${folder_name}.zip" "${folder_name}" || {
    log "ERROR: during zipping files"
    exit 1
}
log "ZIP package ${folder_name}.zip successfully created."


scp_transfer() {
    local source_file="$1"
    local destination="$2"
   
    log "Sending $source_file to remote server..."
   
    sudo scp -o StrictHostKeyChecking=no -i "$PRIVATE_KEY_PATH" "$source_file" "$destination" || {
        log "ERROR: Impossible to send $source_file"
        return 1
    }
}

scp_transfer "${folder_name}.zip" "$distribution@$ip_address:/home/$distribution" || exit 1
rm -f "${folder_name}.zip"

log "Starting docker-compose up on remote server..."
sudo ssh -o StrictHostKeyChecking=no -i "$PRIVATE_KEY_PATH" "$distribution@$ip_address" "unzip ${folder_name}.zip && cd ${folder_name} && docker-compose up --force-recreate -d && cd .. && rm -rf ${folder_name}.zip ${folder_name}" || {
    log "ERROR: Failed executing composition on remote server"
    exit 1
}