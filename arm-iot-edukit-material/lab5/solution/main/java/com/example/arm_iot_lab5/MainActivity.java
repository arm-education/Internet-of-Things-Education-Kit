package com.example.arm_iot_lab5;

import android.Manifest;
import android.annotation.SuppressLint;
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
import android.os.Bundle;

import androidx.activity.EdgeToEdge;

import com.google.android.material.snackbar.Snackbar;

import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Handler;
import android.util.Log;
import android.view.View;

import androidx.core.app.ActivityCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.example.arm_iot_lab5.databinding.ActivityMainBinding;

import android.view.Menu;
import android.view.MenuItem;
import android.widget.ArrayAdapter;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private AppBarConfiguration appBarConfiguration;
    private ActivityMainBinding binding;

    private BluetoothAdapter bluetoothAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        ViewCompat.setOnApplyWindowInsetsListener(binding.main, (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        ActivityCompat.requestPermissions(this, new String[] {
                Manifest.permission.BLUETOOTH,
                Manifest.permission.BLUETOOTH_ADMIN,
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.ACCESS_COARSE_LOCATION,
                Manifest.permission.ACCESS_FINE_LOCATION
        }, 2);

        BluetoothManager bluetoothManager= (BluetoothManager)getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = bluetoothManager.getAdapter();

        MainActivity mainActivity = this;

        binding.textView.setText("Waiting to connect...");

        binding.button.setOnClickListener(new View.OnClickListener() {
            @SuppressLint("MissingPermission")
            @Override
            public void onClick(View v) {
                Log.d("MainActivity", "Connecting to bluetooth");

                if (!bluetoothAdapter.isEnabled()) {
                    Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                    startActivityForResult(enableBtIntent, 1);
                }

                ArrayAdapter<String> devices = new ArrayAdapter<String>(mainActivity, R.layout.list_item);
                Map<String, BluetoothDevice> discoveredDevices = new HashMap<>();

                BluetoothLeScanner scanner = bluetoothAdapter.getBluetoothLeScanner();

                binding.textView.setText("Scanning...");
                scanner.startScan(new ScanCallback() {
                    @Override
                    public void onScanResult(int callbackType, ScanResult result) {
                        BluetoothDevice device = result.getDevice();

                        if (device != null && !discoveredDevices.containsKey(device.getAddress())) {
                            String name = "(unknown)";
                            if (device.getName() != null) {
                                name = device.getName();
                            }

                            devices.add(device.getAddress() + " " + name);
                            discoveredDevices.put(device.getAddress(), device);
                        }
                    }
                });

                AlertDialog.Builder b = new AlertDialog.Builder(mainActivity);
                b.setTitle("Select device");
                b.setSingleChoiceItems(devices, 0, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Log.d("MainActivity", "Selected device " + which + ": " + devices.getItem(which));

                        scanner.stopScan((ScanCallback) null);
                        dialog.dismiss();

                        BluetoothDevice d = discoveredDevices.get(devices.getItem(which).substring(0, 17));
                        if (d != null) {
                            Log.d("MainActivity", "Connecting to device " + d.getAddress());
                            BluetoothGatt g = d.connectGatt(mainActivity, false, new BluetoothGattCallback() {
                                @Override
                                public void onCharacteristicChanged(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value) {
                                    int heartRate = (int)value[1];

                                    Log.d("MainActivity", "New char value: " + heartRate);

                                    mainActivity.runOnUiThread(new Runnable() {
                                        @Override
                                        public void run() {
                                            binding.textView.setText(Integer.toString(heartRate));
                                        }
                                    });
                                }

                                @Override
                                public void onServicesDiscovered(BluetoothGatt gatt, int status) {
                                    BluetoothGattService heartRateService = gatt.getService(UUID.fromString("0000180d-0000-1000-8000-00805f9b34fb"));
                                    if (heartRateService!=null) {
                                        BluetoothGattCharacteristic heartRateChar = heartRateService.getCharacteristic(UUID.fromString("00002a37-0000-1000-8000-00805f9b34fb"));
                                        if (heartRateChar != null) {
                                            gatt.setCharacteristicNotification(heartRateChar, true);

                                            BluetoothGattDescriptor descr = heartRateChar.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"));
                                            if (descr != null) {
                                                descr.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                                                gatt.writeDescriptor(descr);
                                            }
                                        }
                                    }
                                }
                            });

                            new Handler().postDelayed(new Runnable() {
                                @Override
                                public void run() {
                                    binding.textView.setText("Discovering...");
                                    g.discoverServices();
                                }
                            }, 1000);
                        }
                    }
                });

                b.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        scanner.stopScan((ScanCallback) null);
                    }
                });
                b.show();
            }
        });
    }
}