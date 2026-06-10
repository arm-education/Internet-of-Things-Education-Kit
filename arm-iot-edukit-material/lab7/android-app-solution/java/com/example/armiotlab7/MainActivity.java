package com.example.armiotlab7;

import android.app.Activity;
import android.os.Bundle;

import androidx.activity.EdgeToEdge;

import com.google.android.material.snackbar.Snackbar;

import androidx.appcompat.app.AppCompatActivity;

import android.view.View;

import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.navigation.NavController;
import androidx.navigation.Navigation;
import androidx.navigation.ui.AppBarConfiguration;
import androidx.navigation.ui.NavigationUI;

import com.example.armiotlab7.databinding.ActivityMainBinding;

import android.view.Menu;
import android.view.MenuItem;

public class MainActivity extends AppCompatActivity {

    private AppBarConfiguration appBarConfiguration;
    private ActivityMainBinding binding;

    private AppLogic logic;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);

        MainActivity mainActivity = this;
        logic = new AppLogic(mainActivity);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        ViewCompat.setOnApplyWindowInsetsListener(binding.main, (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        binding.connectButton.setOnClickListener((view) -> {
            logic.attemptConnect(new SensorValueUpdatedCallback() {
                @Override
                public void onTemperatureChanged(float temperature) {
                    mainActivity.runOnUiThread(() -> {
                        binding.temperatureText.setText(Float.toString(temperature));
                    });
                }

                @Override
                public void onPressureChanged(float pressure) {
                    mainActivity.runOnUiThread(() -> {
                        binding.pressureText.setText(Float.toString(pressure));
                    });
                }

                @Override
                public void onHumidityChanged(float humidity) {
                    mainActivity.runOnUiThread(() -> {
                        binding.humidityText.setText(Float.toString(humidity));
                    });
                }
            });
        });

        logic.requestPermissions();
    }
}