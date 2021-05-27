package com.kevin.player.media

import android.view.Surface
import android.view.SurfaceHolder

/**
 * Created by wxk on 2021/5/18.
 */
typealias OnPreparedBack=()->Unit
typealias OnErrorBack=(message:String)->Unit
class MediaPlayer : SurfaceHolder.Callback{
    init {
        System.loadLibrary("native-lib")
    }
    private var dataSource :String= ""
    private var onPreparedBack:OnPreparedBack = {

    }
    private var onErrorBack:OnErrorBack = {

    }
    public fun setDataSource(dataSource:String){
        this.dataSource = dataSource
    }

    public fun prepare(){
        nativePrepare(dataSource)
    }

    public fun start(){
        nativeStart()
    }

    public fun stop(){
        nativeStop()
    }

    public fun release(){
        nativeRelease()
    }

    public fun onPrepared(){
        onPreparedBack()
    }

    public fun onError(code:Int){
        onErrorBack("")
    }

    public fun setListener(onPreparedBack: OnPreparedBack,onErrorBack:OnErrorBack){
        this.onPreparedBack = onPreparedBack
        this.onErrorBack = onErrorBack
    }

    override fun surfaceCreated(p0: SurfaceHolder?) {
    }

    override fun surfaceChanged(p0: SurfaceHolder?, p1: Int, p2: Int, p3: Int) {
        p0?.surface?.let {
            nativeSetSurface(it)
        }

    }

    override fun surfaceDestroyed(p0: SurfaceHolder?) {
    }

    external fun nativePrepare(dataSource: String)
    external fun nativeStart()
    external fun nativeRelease()
    external fun nativeStop()
    external fun nativeOnPrepared()
    external fun getFFmpegVersion():String
    external fun nativeSetSurface(surface:Surface)
}