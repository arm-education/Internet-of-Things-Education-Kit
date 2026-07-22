package com.example.armiotlab7;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.util.Log;
import android.widget.ArrayAdapter;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

import java.util.ArrayDeque;
import java.util.HashSet;
import java.util.Queue;
import java.util.Set;
import java.util.UUID;

public class AppLogic {
    private class DiscoveredBluetoothDevice {
        private BluetoothDevice device;

        public DiscoveredBluetoothDevice(BluetoothDevice device) {
            this.device = device;
        }

        public BluetoothDevice getDevice() {
            return device;
        }

        @SuppressLint("MissingPermission")
        @Override
        @NonNull
        public String toString() {
            String name = "(unknown)";
            if (device.getName() != null) {
                name = device.getName();
            }

            return device.getAddress() + ": " + name;
        }
    }

    private Activity mainActivity;
    private BluetoothManager btManager;
    private BluetoothAdapter btAdapter;
    private BluetoothLeScanner btScanner;

    private Queue<UUID> notificationQueue = new ArrayDeque<>();

    public AppLogic(Activity mainActivity) {
        this.mainActivity = mainActivity;
        this.btManager = (BluetoothManager) mainActivity.getSystemService(Context.BLUETOOTH_SERVICE);
        this.btAdapter = this.btManager.getAdapter();
        this.btScanner = this.btAdapter.getBluetoothLeScanner();
    }

    public void requestPermissions() {
        ActivityCompat.requestPermissions(this.mainActivity, new String[]{
                Manifest.permission.BLUETOOTH,
                Manifest.permission.BLUETOOTH_ADMIN,
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.ACCESS_COARSE_LOCATION,
                Manifest.permission.ACCESS_FINE_LOCATION
        }, 2);
    }

    private void ensureBTEnabled() {
        if (!this.btAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            if (ActivityCompat.checkSelfPermission(this.mainActivity, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                return;
            }

            this.mainActivity.startActivityForResult(enableBtIntent, 1);
        }
    }

    private ArrayAdapter<DiscoveredBluetoothDevice> initiateScan() {
        ArrayAdapter<DiscoveredBluetoothDevice> devices = new ArrayAdapter<DiscoveredBluetoothDevice>(this.mainActivity, R.layout.device_list_item);
        Set<String> discoveredDevices = new HashSet<>();

        if (ActivityCompat.checkSelfPermission(this.mainActivity, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            return null;
        }

        this.btScanner.startScan(new ScanCallback() {
            @Override
            public void onScanResult(int callbackType, ScanResult result) {
                BluetoothDevice device = result.getDevice();

                if (device != null && device.getAddress() != null && !discoveredDevices.contains(device.getAddress())) {
                    devices.add(new DiscoveredBluetoothDevice(device));
                    discoveredDevices.add(device.getAddress());
                }
            }
        });

        return devices;
    }

    @SuppressLint("MissingPermission")
    private void displayDeviceChooser(ArrayAdapter<DiscoveredBluetoothDevice> devices, SensorValueUpdatedCallback valueUpdatedCallback) {
        AlertDialog.Builder chooserDialog = new AlertDialog.Builder(this.mainActivity);
        chooserDialog.setTitle("Select device");
        chooserDialog.setSingleChoiceItems(devices, 0, (dialog, which) -> {
            DiscoveredBluetoothDevice selectedDevice = devices.getItem(which);
            if (selectedDevice != null) {
                connectToDevice(selectedDevice.getDevice(), valueUpdatedCallback);
                btScanner.stopScan((ScanCallback) null);

                dialog.dismiss();
            }
        });

        chooserDialog.setNegativeButton("Cancel", (dialog, which) -> {
            btScanner.stopScan((ScanCallback) null);
        });

        chooserDialog.show();
    }

    @SuppressLint("MissingPermission")
    private void processOneNotificationActivation(BluetoothGatt gatt, BluetoothGattService service) {
        if (notificationQueue.isEmpty()) {
            return;
        }

        UUID characteristicUuid = notificationQueue.remove();

        BluetoothGattCharacteristic characteristic = service.getCharacteristic(characteristicUuid);
        if (characteristic != null) {
            BluetoothGattDescriptor descr = characteristic.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"));
            if (descr != null) {
                gatt.setCharacteristicNotification(characteristic, true);
                gatt.writeDescriptor(descr, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
            }
        }
    }

    private void activateServiceNotifications(BluetoothGatt gatt, BluetoothGattService service) {
        UUID[] characteristics = new UUID[]{
                UUID.fromString("00002a6e-0000-1000-8000-00805f9b34fb"),
                UUID.fromString("00002a6d-0000-1000-8000-00805f9b34fb"),
                UUID.fromString("00002a6f-0000-1000-8000-00805f9b34fb")
        };

        for (UUID characteristicUuid : characteristics) {
            notificationQueue.add(characteristicUuid);
        }

        processOneNotificationActivation(gatt, service);
    }

    private void connectToDevice(BluetoothDevice device, SensorValueUpdatedCallback valueUpdatedCallback) {
        @SuppressLint("MissingPermission") BluetoothGatt gatt = device.connectGatt(this.mainActivity, false, new BluetoothGattCallback() {
            @Override
            public void onCharacteristicChanged(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value) {
                Log.d("AppLogic", "Characteristic changed: " + characteristic.getUuid());

                if (characteristic.getUuid().equals(UUID.fromString("00002a6e-0000-1000-8000-00805f9b34fb"))) {
                    // Temperature 16-bit signed
                    int temperature = value[0] | (int) value[1] << 8;
                    valueUpdatedCallback.onTemperatureChanged((float) temperature);
                } else if (characteristic.getUuid().equals(UUID.fromString("00002a6d-0000-1000-8000-00805f9b34fb"))) {
                    // Pressure 32-bit unsigned
                    int pressure = value[0] | (int) value[1] << 8 | (int) value[2] << 16 | (int) value[3] << 24;
                    valueUpdatedCallback.onPressureChanged((float) pressure);
                } else if (characteristic.getUuid().equals(UUID.fromString("00002a6f-0000-1000-8000-00805f9b34fb"))) {
                    // Humidity 16-bit unsigned
                    int humidity = value[0] | (int) value[1] << 8;
                    valueUpdatedCallback.onHumidityChanged((float) humidity);
                }
            }

            @Override
            public void onServicesDiscovered(BluetoothGatt gatt, int status) {
                BluetoothGattService environmentalSensorService = gatt.getService(UUID.fromString("0000181a-0000-1000-8000-00805f9b34fb"));
                if (environmentalSensorService != null) {
                    activateServiceNotifications(gatt, environmentalSensorService);
                }
            }

            @Override
            public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
                Log.d("AppLogic", "onDescriptorWrite: " + descriptor);

                processOneNotificationActivation(gatt, descriptor.getCharacteristic().getService());
            }
        });

        new Handler().postDelayed(new Runnable() {
            @SuppressLint("MissingPermission")
            @Override
            public void run() {
                gatt.discoverServices();
            }
        }, 1000);
    }

    public void attemptConnect(SensorValueUpdatedCallback valueUpdatedCallback) {
        this.ensureBTEnabled();
        ArrayAdapter<DiscoveredBluetoothDevice> devices = this.initiateScan();
        this.displayDeviceChooser(devices, valueUpdatedCallback);
    }
}
