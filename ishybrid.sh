#!/bin/sh

# Function to convert hex to binary
hex_to_bin() {
  echo "obase=2; ibase=16; $1" | bc
}

# Function to convert hex to uppercase without 0x prefix
hex_to_upper() {
  echo "$1" | tr 'a-f' 'A-F' | sed 's/^0x//'
}

# Function to get the range of core numbers
get_core_range() {
  local cores="$1"
  local first_core=$(echo $cores | awk '{print $1}')
  local last_core=$(echo $cores | awk '{print $NF}')
  echo "$first_core-$last_core"
}

# Get CPU vendor
cpu_vendor=$(awk -F ': ' '/vendor_id/ {print $2; exit}' /proc/cpuinfo)

# Get the number of CPU cores
cpu_count=$(nproc)

if [ "$cpu_vendor" = "GenuineIntel" ]; then
  # Check if the CPU is a hybrid design
  edx_value=$(cpuid -r -l 0x07 | awk '/edx/ {print $NF; exit}' | sed 's/.*=//; s/^0x//')
  edx_value_upper=$(hex_to_upper $edx_value)
  edx_bin=$(hex_to_bin $edx_value_upper)

  # Check the 15th bit (from the right, 0-indexed)
  is_hybrid=$(echo $edx_bin | rev | cut -c 15)

  if [ "$is_hybrid" = "1" ]; then
    echo "This is a hybrid CPU."

    # Initialize core ranges
    p_cores=""
    e_cores=""

    # Get all leaf 0x1a lines
    leaf_1a_lines=$(cpuid -r -l 0x1a | awk '/eax/ {print $3}')

    for i in $(seq 0 $((cpu_count - 1))); do
      core_type=$(echo "$leaf_1a_lines" | sed -n "$((i + 1))p" | cut -d'=' -f2 | sed 's/^0x//')
      core_type_upper=$(hex_to_upper $core_type)
      core_prefix=$(echo $core_type_upper | cut -c 1-2)
      case $core_prefix in
        20)
          e_cores="$e_cores $i"
          ;;
        40)
          p_cores="$p_cores $i"
          ;;
      esac
    done

    p_core_count=$(echo $p_cores | wc -w)
    e_core_count=$(echo $e_cores | wc -w)
    p_core_range=$(get_core_range "$p_cores")
    e_core_range=$(get_core_range "$e_cores")

    echo "P-core x$p_core_count: $p_core_range"
    echo "E-core x$e_core_count: $e_core_range"
  else
    echo "This is not a hybrid CPU."
    echo "Core x$cpu_count: 0-$((cpu_count - 1))"
  fi
else
  echo "This is not a hybrid CPU."
  echo "Core x$cpu_count: 0-$((cpu_count - 1))"
fi
