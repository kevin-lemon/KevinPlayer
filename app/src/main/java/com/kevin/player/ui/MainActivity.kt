package com.kevin.player.ui

import android.app.Activity
import android.os.Bundle
import android.os.Environment
import android.util.Log
import android.view.SurfaceView
import android.widget.TextView
import android.widget.Toast
import com.kevin.player.R
import com.kevin.player.media.MediaPlayer
import java.io.File
import java.nio.file.Path

/**
 * Created by wxk on 2021/5/18.
 */
class MainActivity :Activity(){
    lateinit var player:MediaPlayer
    var tvState:TextView?=null
    var surfaceView : SurfaceView?=null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        player = MediaPlayer()
        tvState = findViewById(R.id.tv_state)
        surfaceView = findViewById(R.id.surfaceView)
        surfaceView?.holder?.addCallback(player)
        var file = File(Environment.getExternalStorageDirectory().toString() + File.separator + "demo.mp4").absolutePath
        Log.d("wxk:",file)
        player.setDataSource(file)
        player.setListener(onPreparedBack = {
            runOnUiThread {
                Toast.makeText(this,"准备成功,开始播放",Toast.LENGTH_SHORT).show()
                player.start()
            }
        },onErrorBack = {
            tvState?.text = it
        })
    }

    override fun onResume() {
        super.onResume()
        player.prepare()
    }

    override fun onStop() {
        super.onStop()
        player.stop()
    }

    override fun onDestroy() {
        super.onDestroy()
        player.release()
    }
}