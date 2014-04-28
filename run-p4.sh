#!/bin/bash

# Terminal colors
declare -r WHITE='\E[37m'
declare -r GRAY='\E[38m'
declare -r BLUE='\E[34m'
declare -r CYAN='\E[36m'
declare -r YELLOW='\E[33m'
declare -r GREEN='\E[32m'
declare -r RED='\E[31m'
declare -r MAGENTA='\E[35m'

# Path of ns3 directory for running waf
declare NS3_PATH=~/myRepos/ns-allinone-3.19/ns-3.19/

# Default data file name
declare DATAFILE=p4.data

# Run through simulations
# $1 User-specified data file name
Run()
{
    trap exit 1 SIGINT

    declare -l fileHeader="Time #inf #Nodes %Vul %Inf #Conn payload"

    # Percent of nodes infectable
    declare -a percInf=("0.1"
                        "0.4"
                        "0.7"
                        "1.0"
                        )

    # Payload
    declare -a payload=("512"
                        "1024"
                        "2048"
                        "5120"
                        )

    # Simultaneous Connections
    declare -a conn=("1" "2" "4" "8" "16" "32" "48" "64" "128")

    echo -e "${BLUE}Running P4 Simluation... ${GRAY}\n"

    # Go to ns3 directory to run waf
    cd ${NS3_PATH}

    # Create data file, optionally specified by user
    if [ "${1}" != "" ]; then
        touch $1
    else
        touch ${DATAFILE}
    fi

    echo "${fileHeader}" > ${DATAFILE}

    echo "Running TCP experiments"
    
    ###################
    # TCP EXperiments #
    ###################
    for (( pinf=0; pinf<${#percInf[@]}; ++pinf ))
    do
        for (( load=0; load<${#payload[@]}; ++load ))
        do
            ./waf --run "ns3-wormsim --logTop=1 --wormtype=2 \
                         --vulnerability=${percInf[$pinf]} \
                         --payload=${payload[$load]}"
        done
    done

    echo -e "${GREEN}Finished simulation! ${WHITE}\n"

}

# Clean out .xml animation files
# $1: File extension to delete
Clean()
{
    cd ${NS3_PATH}

    xmlCount=`find ./ -maxdepth 1 -name "*.xml" | wc -l`
    dataCount=`find ./ -maxdepth 1 -name "*.data" | wc -l`

    if [ "$1" == "" ]; then
        if [ ${xmlCount} != 0 ]; then
            rm *.xml
            echo "Removed ${xmlCount} .xml files"
        else
            echo "No .xml files found"
        fi
        if [ ${dataCount} != 0 ]; then
            rm *.data
            echo "Removed ${dataCount} .data files"
        else
            echo "No .data files found"
        fi    

    else
        count=`find ./ -maxdepth 1 -name "*${1}" | wc -l`
        if [ ${count} != 0 ]; then
            rm *.${1}
            echo "Removed ${count} .${1} files"
        else
            echo "No .${1} files found"
        fi
    fi
}

# Print usage instructions
ShowUsage()
{
    echo -e "${GRAY}\n"
    echo "Script to run Network Simulator 3 simulations"
    echo "Usage: ./run-p1.sh <COMMAND> [OPTION]"
    echo "Commands:"
    echo "   -r|--run                     Run simulation"
    echo "       OPTIONS: dataFileName    Run simulation and save data in file named <dataFileName>"
    echo "   -c|--clean                   Clean out the .xml animation and .data files"
    echo "       OPTIONS: file extension  Clean out .<file extension> files"
    echo "   -h|--help                    Show this help message"
    echo "Examples:"
    echo "    ./run-p1.sh -r"
    echo "    ./run-p1.sh -r p1-2014.data"
    echo "    ./run-p1.sh -c"
    echo "    ./run-p1.sh -c xml"
    echo -e "${WHITE}\n"
}

main()
{
    case "$1" in
    '-r'|'--run' )
        Run $2
    ;;

    '-c'|'--clean' )
        Clean $2
n    ;;

    *)
        ShowUsage
        exit 1
    ;;
    esac

    exit 0
}

main "$@"
